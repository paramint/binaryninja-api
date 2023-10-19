#include "mainfunctiondetector.h"
#include "lowlevelilinstruction.h"

using namespace BinaryNinja;
using namespace std;

// TODO read the list from ui.files.navigation.preferMain
vector<string> mainFunctionNames = {"main", "wmain", "WinMain", "wWinMain"};


struct WinMainDetectionInfo
{
	bool found = false;
	bool method1 = false;
	bool method2 = false;
	bool method3 = false;
	uint64_t address = 0;
	string reason;
};


bool IsDLL(const Ref<BinaryView>& bv)
{
	auto sym = bv->GetSymbolByRawName("__coff_header");
	if (!sym)
		return false;

	DataVariable var;
	if (!bv->GetDataVariableAtAddress(sym->GetAddress(), var))
		return false;

	BinaryReader reader(bv);
	const auto offset_characteristics = 0x16;
	reader.Seek(sym->GetAddress() + offset_characteristics);

	uint16_t result = 0;
	if (!reader.TryRead16(result))
		return false;

	return (result & 0x2000) == 0x2000;
}


bool IsDriver(const Ref<BinaryView>& bv)
{
	auto sym = bv->GetSymbolByRawName("__pe64_optional_header");
	if (!sym)
		return false;

	DataVariable var;
	if (!bv->GetDataVariableAtAddress(sym->GetAddress(), var))
		return false;

	BinaryReader reader(bv);
	const auto offset_subsystem = 0x44;
	reader.Seek(sym->GetAddress() + offset_subsystem);

	uint16_t result = 0;
	if (!reader.TryRead16(result))
		return false;

	return result == 1;
}


static bool StringEndsWith(const string& text, const string& end)
{
	if (end.size() > text.size())
		return false;

	return text.substr(text.size() - end.size()) == end;
}


bool IsExitFunction(const Ref<BinaryView>& view, uint64_t address)
{
	std::vector<string> exitFunctions = {"exit", "_exit", "_o__cexit", "_o_exit", "_cexit", "common_exit", "doexit"};
	auto sym = view->GetSymbolByAddress(address);
	if (sym)
	{
		auto name = sym->GetShortName();
		for (const auto& funcName: exitFunctions)
			if (name == funcName)
				return true;
	}

	auto func = view->GetAnalysisFunction(view->GetDefaultPlatform(), address);
	if (!func)
		return false;

	auto llil = func->GetLowLevelIL();
	if (!llil)
		return false;

	auto ssa = llil->GetSSAForm();
	if (!ssa)
		return false;

	if (ssa->GetInstructionCount() == 0)
		return false;

	auto il = ssa->GetInstruction(ssa->GetInstructionCount() - 1);
	if (il.operation != LLIL_TAILCALL_SSA)
		return false;

	auto dest = il.GetDestExpr<LLIL_TAILCALL_SSA>();
	uint64_t stubAddress = dest.GetValue().value;

	auto stubSym = view->GetSymbolByAddress(stubAddress);
	if (stubSym)
	{
		auto name = stubSym->GetShortName();
		for (const auto& funcName: exitFunctions)
			if (name == funcName)
				return true;
		return false;
	}
	else
	{
		sym = view->GetSymbolByRawName("_exit");
		if (!sym)
			return false;

		func = view->GetAnalysisFunction(view->GetDefaultPlatform(), sym->GetAddress());
		if (!func)
			return false;

		llil = func->GetLowLevelIL();
		if (!llil)
			return false;

		ssa = llil->GetSSAForm();
		if (!ssa)
			return false;

		if (ssa->GetInstructionCount() == 0)
			return false;

		il = ssa->GetInstruction(ssa->GetInstructionCount() - 1);
		if (il.operation != LLIL_TAILCALL_SSA)
			return false;

		dest = il.GetDestExpr<LLIL_TAILCALL_SSA>();
		return dest.GetValue().value == stubAddress;
	}
	return false;
}


std::optional<SSARegister> GetRegisterSetInBlocks(const Ref<BasicBlock>& block, uint32_t reg,
												  uint64_t maxSearchIndex = -1)
{
	auto function = block->GetLowLevelILFunction();
	auto searchStart = std::min<uint64_t>(block->GetEnd() - 1, maxSearchIndex);
	if (searchStart == -1)
		return std::nullopt;

	for (int64_t i = searchStart; i > block->GetStart(); i--)
	{
		auto il = function->GetInstruction(i - 1);
		switch (il.operation)
		{
		case LLIL_SET_REG_SSA:
		{
			if (il.GetDestSSARegister<LLIL_SET_REG_SSA>().reg == reg)
				return il.GetDestSSARegister<LLIL_SET_REG_SSA>();
			break;
		}
		case LLIL_REG_PHI:
		{
			if (il.GetDestSSARegister<LLIL_REG_PHI>().reg == reg)
				return il.GetDestSSARegister<LLIL_REG_PHI>();
			break;
		}
		default:
			break;
		}
	}
	return nullopt;
}


std::set<SSARegister> GetTargetSSARegisters(const Ref<LowLevelILFunction>& ssa)
{
	std::set<SSARegister> result;

	auto source = ssa->GetFunction();
	if (!source)
		return result;

	auto callingConvention = source->GetCallingConvention();
	if (!callingConvention)
		return result;

	auto argRegs = callingConvention->GetIntegerArgumentRegisters();
	if (argRegs.empty())
		return result;

	auto firstArgReg = argRegs[0];

	auto returnReg = callingConvention->GetIntegerReturnValueRegister();

	for (size_t i = 0; i < ssa->GetInstructionCount(); i++)
	{
		auto il = ssa->GetInstruction(i);
		switch (il.operation)
		{
		case LLIL_CALL_SSA:
		{
			auto dest = il.GetDestExpr<LLIL_CALL_SSA>();
			if (!IsExitFunction(ssa->GetFunction()->GetView(), dest.GetValue().value))
				break;

			auto params = il.GetParameterExprs<LLIL_CALL_SSA>();
			if (params.size() > 0)
			{
				// this is the easy case where MLIL has already figured out the parameter
				auto p = params[0];
				if (p.operation == LLIL_REG_SSA)
					result.emplace(p.GetSourceSSARegister<LLIL_REG_SSA>());
			}
			else
			{
				// slightly harder case we need to figure out the parameter ourselves
				auto block = ssa->GetBasicBlockForInstruction(i);
				if(!block)
					break;

				auto ssaReg = GetRegisterSetInBlocks(block, firstArgReg, i);
				if (ssaReg.has_value())
				{
					result.emplace(ssaReg.value());
				}
				else
				{
					// search incoming blocks
					for (const auto& edge: block->GetIncomingEdges())
					{
						ssaReg = GetRegisterSetInBlocks(edge.target, firstArgReg);
						if (ssaReg.has_value())
						{
							result.emplace(ssaReg.value());
						}
					}
				}
			}
			break;
		}
		case LLIL_RET:
		{
			auto block = ssa->GetBasicBlockForInstruction(i);
			if(!block)
				break;

			auto ssaReg = GetRegisterSetInBlocks(block, returnReg, i);
			if (ssaReg.has_value())
			{
				result.emplace(ssaReg.value());
			}
			else
			{
				// search incoming blocks
				for (const auto& edge: block->GetIncomingEdges())
				{
					ssaReg = GetRegisterSetInBlocks(edge.target, returnReg);
					if (ssaReg.has_value())
					{
						result.emplace(ssaReg.value());
					}
				}
			}
			break;
		}
		default:
			break;
		}
	}
	return result;
}


bool SinkToReturn(const std::set<SSARegister>& targetRegs, const Ref<LowLevelILFunction>& func,
				  const SSARegister& ssaReg, std::set<uint32_t>& seen)
{
	auto instrs = func->GetSSARegisterUses(ssaReg);
	for (const auto& index: instrs)
	{
		auto il = func->GetInstruction(index);
		switch (il.operation)
		{
		case LLIL_SET_REG_SSA:
		{
			auto src = il.GetSourceExpr<LLIL_SET_REG_SSA>();

			bool matched = false;
			if (src.operation == LLIL_REG_SSA)
				matched = true;
			else if (src.operation == LLIL_ZX)
			{
				auto src2 = src.GetSourceExpr<LLIL_ZX>();
				if ((src2.operation == LLIL_REG_SSA) || (src2.operation == LLIL_REG_SSA_PARTIAL))
					matched = true;
			}
			else if (src.operation == LLIL_SX)
			{
				auto src2 = src.GetSourceExpr<LLIL_SX>();
				if ((src2.operation == LLIL_REG_SSA) || (src2.operation == LLIL_REG_SSA_PARTIAL))
					matched = true;
			}
			if (!matched)
				break;

			if (seen.find(il.instructionIndex) != seen.end())
				return false;

			seen.emplace(il.instructionIndex);
			auto dest = il.GetDestSSARegister<LLIL_SET_REG_SSA>();
			if (targetRegs.find(dest) != targetRegs.end())
				return true;

			if (SinkToReturn(targetRegs, func, dest, seen))
				return true;
			break;
		}
		case LLIL_REG_PHI:
		{
			if (seen.find(il.instructionIndex) != seen.end())
				return false;

			seen.emplace(il.instructionIndex);
			auto dest = il.GetDestSSARegister<LLIL_REG_PHI>();
			if (targetRegs.find(dest) != targetRegs.end())
				return true;

			if (SinkToReturn(targetRegs, func, dest, seen))
				return true;
			break;
		}
		case LLIL_STORE_SSA:
		{
			auto dest = il.GetDestExpr<LLIL_STORE_SSA>();
			auto storeAddress = dest.GetValue().value;
			for (size_t i = 0; i < func->GetInstructionCount(); i++)
			{
				auto il2 = func->GetInstruction(i);
				if (il2.operation != LLIL_SET_REG_SSA)
					continue;

				auto src = il2.GetSourceExpr<LLIL_SET_REG_SSA>();
				if (src.operation == LLIL_SX)
					src = src.GetSourceExpr<LLIL_SX>();
				else if (src.operation == LLIL_ZX)
					src = src.GetSourceExpr<LLIL_ZX>();

				if (src.operation != LLIL_LOAD_SSA)
					continue;

				src = src.GetSourceExpr<LLIL_LOAD_SSA>();
				if ((src.GetValue().state != ConstantValue) && (src.GetValue().state != ConstantPointerValue))
					continue;
				if (src.GetValue().value != storeAddress)
					continue;
				if (seen.find(il.instructionIndex) != seen.end())
					return false;
				seen.emplace(il.instructionIndex);
				auto dest2 = il2.GetDestSSARegister<LLIL_SET_REG_SSA>();
				if (targetRegs.find(dest2) != targetRegs.end())
					return true;
				if (SinkToReturn(targetRegs, func, dest2, seen))
					return true;
			}
		}
		default:
			break;
		}
	}
	return false;
}


std::vector<WinMainDetectionInfo> DetectionMethod1(BinaryView* view, Function* func, LowLevelILFunction* llil)
{
	std::vector<WinMainDetectionInfo> results;

	auto ssa = llil->GetSSAForm();
	if (!ssa)
	{
		WinMainDetectionInfo result;
		result.reason = "No SSA available";
		results.emplace_back(result);
		return results;
	}

	auto callingConvention = func->GetCallingConvention();
	if (!callingConvention)
	{
		WinMainDetectionInfo result;
		result.reason = "No calling convention available";
		results.emplace_back(result);
		return results;
	}

	auto returnReg = callingConvention->GetIntegerReturnValueRegister();
	auto targetRegs = GetTargetSSARegisters(ssa);

	for (size_t i = 0; i < ssa->GetInstructionCount(); i++)
	{
		auto il = ssa->GetInstruction(i);
		switch (il.operation)
		{
		case LLIL_CALL_SSA:
		case LLIL_TAILCALL_SSA:
		{
			auto dest = il.GetDestExpr();
			auto value = dest.GetValue().value;
			auto sym = view->GetSymbolByAddress(value);
			if (sym)
			{
				if (std::find(mainFunctionNames.begin(), mainFunctionNames.end(), sym->GetRawName()) !=
					mainFunctionNames.end())
				{
					WinMainDetectionInfo result;
					result.found = true;
					result.method1 = true;
					result.address = value;
					results.emplace_back(result);
					break;
				}
			}

			auto outputs = il.GetOutputSSARegisters();
			for (const auto& output: outputs)
			{
				if (output.reg != returnReg)
					continue;

				std::set<uint32_t> seen;
				if (SinkToReturn(targetRegs, ssa, output, seen))
				{
					WinMainDetectionInfo result;
					result.found = true;
					result.method1 = true;
					result.address = value;
					results.emplace_back(result);
					break;
				}
			}
		}
		default:
			break;
		}
	}

	return results;
}


static std::optional<std::pair<Ref<BasicBlock>, size_t>> GetCallingBlockAndInstruction(
		BinaryView* view, LowLevelILFunction* llil, const std::string& name)
{
	for (auto block: llil->GetBasicBlocks())
	{
		for (size_t i = block->GetStart(); i < block->GetEnd(); i++)
		{
			auto il = llil->GetInstruction(i);
			if (il.operation != LLIL_CALL)
				continue;
			auto dest = il.GetDestExpr<LLIL_CALL>();
			if ((dest.GetValue().state != ConstantValue) && (dest.GetValue().state != ConstantPointerValue))
				continue;
			auto value = dest.GetValue().value;
			auto sym = view->GetSymbolByAddress(value);
			if (!sym)
				continue;
			if (sym->GetShortName() == name)
				return std::make_pair(block, i);
		}
	}
	return std::nullopt;
}


std::vector<WinMainDetectionInfo> DetectionMethod2(BinaryView* view, Function* func, LowLevelILFunction* llil)
{
	std::vector<WinMainDetectionInfo> results;

	// Find a call to __p___argc
	auto caller = GetCallingBlockAndInstruction(view, llil, "__p___argc");
	if (!caller.has_value())
		return results;

	auto block = caller->first;
	auto idx = caller->second;

	if (!block)
		return results;

	// Find the next LLIL_CALL instruction after the call to __p___argc
	for (size_t i = idx + 1; i < block->GetEnd(); i++)
	{
		auto il = llil->GetInstruction(i);
		if (il.operation != LLIL_CALL)
			continue;
		auto dest = il.GetDestExpr<LLIL_CALL>();
		if ((dest.GetValue().state != ConstantValue) && (dest.GetValue().state != ConstantPointerValue))
			break;
		// If the call destination has no symbol or the symbol name is a known main function name, treat it as main
		auto value = dest.GetValue().value;
		auto sym = view->GetSymbolByAddress(value);
		if ((!sym) ||
			std::find(mainFunctionNames.begin(), mainFunctionNames.end(), sym->GetRawName())
			!= mainFunctionNames.end())
		{
			WinMainDetectionInfo result;
			result.found = true;
			result.method2 = true;
			result.address = value;
			results.emplace_back(result);
			break;
		}
		// We only consider the first LLIL_CALL after the __p___argc. It is not a main function, break out
		break;
	}
	return results;
}


std::vector<WinMainDetectionInfo> DetectionMethod3(BinaryView* view, Function* func, LowLevelILFunction* llil)
{
	std::vector<WinMainDetectionInfo> results;

	auto sym = func->GetSymbol();
	if (!sym)
		return results;

	if (sym->GetShortName() != "invoke_main")
		return results;

	auto ssa = llil->GetSSAForm();
	if (!ssa)
		return results;

	for(int64_t i = ssa->GetInstructionCount(); i >= 0; i--)
	{
		auto il = ssa->GetInstruction(i);
		if (il.operation != LLIL_CALL_SSA)
			continue;

		auto dest = il.GetDestExpr<LLIL_CALL_SSA>();
		auto value = dest.GetValue().value;
		auto mainStub = view->GetAnalysisFunction(view->GetDefaultPlatform(), value);
		if (!mainStub)
			break;
		auto mainStubllil = mainStub->GetLowLevelILIfAvailable();
		if (!mainStubllil)
			break;
		auto mainStubSSA = mainStubllil->GetSSAForm();
		if (!mainStubSSA)
			break;
		if (mainStubSSA->GetInstructionCount() != 1)
			break;
		auto tailCall = mainStubSSA->GetInstruction(0);
		if (tailCall.operation != LLIL_TAILCALL_SSA)
			break;

		auto tailCallDest = tailCall.GetDestExpr<LLIL_TAILCALL_SSA>();
		auto tailCallValue = tailCallDest.GetValue().value;

		WinMainDetectionInfo result;
		result.found = true;
		result.method3 = true;
		result.address = tailCallValue;
		results.emplace_back(result);
		break;
	}

	return results;
}


WinMainDetectionInfo IsCommonMain(BinaryView* view, Function* func, LowLevelILFunction* il)
{
	WinMainDetectionInfo result;

	bool isCalledByEntry = false;
	auto entryPoint = view->GetEntryPoint();
	auto refs = view->GetCodeReferences(func->GetStart());
	for (const auto& ref: refs)
	{
		// The candidate function must be a direct callee of the entry point
		// TODO: if this is relaxed, not only we could get false positives, the detection algorithm also hangs on
		// certain functions because it could be requesting the LLIL of a function which has not been analyzed
		if (ref.func->GetStart() == entryPoint)
		{
			isCalledByEntry = true;
			break;
		}
	}

	std::vector<WinMainDetectionInfo> candidates;
	if (isCalledByEntry)
	{
		// detection method 1 and 2 require that the current function is called by the entry point
		auto results1 = DetectionMethod1(view, func, il);
		auto results2 = DetectionMethod2(view, func, il);
		candidates.insert(candidates.end(), results1.begin(), results1.end());
		candidates.insert(candidates.end(), results2.begin(), results2.end());
	}

	auto results3 = DetectionMethod3(view, func, il);
	candidates.insert(candidates.end(), results3.begin(), results3.end());

	if (candidates.empty())
	{
		result.reason = "No candidates found";
	}
	else if (candidates.size() == 1)
	{
		result = candidates[0];
		result.reason = "Found common main";
	}
	else
	{
		auto address = candidates[0].address;
		bool ok = true;
		for (size_t i = 1; i < candidates.size(); i++)
		{
			if (candidates[i].address != address)
			{
				ok = false;
				break;
			}
		}

		if (!ok)
		{
			result.found = false;
			result.reason = "Multiple different candidates found";
		}
		else
		{
			result.found = true;
			for (const auto& candidate: candidates)
			{
				result.method1 |= candidate.method1;
				result.method2 |= candidate.method2;
				result.method3 |= candidate.method3;
			}
			result.reason = "Found common main";
			result.address = address;
		}
	}

	return result;
}


WinMainFunctionRecognizer::WinMainFunctionRecognizer(Ref<Platform> platform) : m_platform(platform)
{
	auto settings = Settings::Instance();
	m_mainFunctionNames = settings->Get<vector<string>>("ui.files.navigation.mainSymbols");
}


bool WinMainFunctionRecognizer::MainFunctionDetectionDone(BinaryNinja::BinaryView *view)
{
	auto data = view->QueryMetadata("__BN_main_function_address");
	if (data && data->IsUnsignedInteger())
		return true;

	data = view->QueryMetadata("__BN_main_function_not_found");
	if (data && data->IsBoolean())
		return true;
	return false;
}


static bool AddEntryCalleeToPriorityQueue(BinaryView* bv, Function* entry)
{
	auto callSites = entry->GetCallSites();
	if (callSites.size() > 2)
	{
		// The entry point has more than two callees, cannot find main
		auto notFound = new Metadata(true);
		bv->StoreMetadata("__BN_main_function_not_found", notFound, true);
		return false;
	}

	std::set<uint64_t> calleeAddresses;
	for (const auto& callSite: callSites)
	{
		for (auto addr: bv->GetCallees(callSite))
			calleeAddresses.emplace(addr);
	}

	for (const uint64_t addr: calleeAddresses)
	{
		auto func = bv->GetAnalysisFunction(bv->GetDefaultPlatform(), addr);
		if (!func)
			continue;
		func->RequestAdvancedAnalysisData();
	}
	return true;
}


bool WinMainFunctionRecognizer::RecognizeLowLevelIL(BinaryView* view, Function* func, LowLevelILFunction* il)
{
	// Make sure the function belongs to the desired platform. Platform specific function recognizers
	// are not a feature so this was registered for the architecture as a whole.
	if (func->GetPlatform() != m_platform)
		return false;

	// main function has either been found or could not be found, returning
	if (MainFunctionDetectionDone(view))
		return true;

	auto entryFunc = view->GetAnalysisEntryPoint();
	if (!entryFunc)
		return false;

	if (func->GetStart() == entryFunc->GetStart())
	{
		// Add the callees of the entry function into the priority queue, so they get analyzed sooner and the detection
		// can finish faster
		if (AddEntryCalleeToPriorityQueue(view, entryFunc))
			return false;

		if (IsDLL(view) || IsDriver(view))
		{
			// Do not detect main for DLL and driver
			auto notFound = new Metadata(true);
			view->StoreMetadata("__BN_main_function_not_found", notFound, true);
			return false;
		}
	}

	auto info = IsCommonMain(view, func, il);
	if (info.found)
	{
		LogDebug("main function found in function: 0x%llx", func->GetStart());
		auto entry = new Metadata(info.address);
		view->StoreMetadata("__BN_main_function_address", entry, true);
		return true;
	}

	return false;
}
