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
//		case LLIL_RET:
//		{
//			auto block = ssa->GetBasicBlockForInstruction(i);
//			if(!block)
//				break;
//
//			auto ssaReg = GetRegisterSetInBlocks(block, returnReg, i);
//			if (ssaReg.has_value())
//			{
//				result.emplace(ssaReg.value());
//			}
//			else
//			{
//				// search incoming blocks
//				for (const auto& edge: block->GetIncomingEdges())
//				{
//					ssaReg = GetRegisterSetInBlocks(edge.target, returnReg);
//					if (ssaReg.has_value())
//					{
//						result.emplace(ssaReg.value());
//					}
//				}
//			}
//			break;
//		}
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
				if (src.GetValue().state != ConstantPointerValue)
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


std::vector<WinMainDetectionInfo> DetectionMethod1(const Ref<LowLevelILFunction>& ssa, uint32_t returnReg)
{
	std::vector<WinMainDetectionInfo> results;

	auto function = ssa->GetFunction();
	if (!function)
		return results;

	auto view = function->GetView();
	if (!view)
		return results;

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


std::vector<WinMainDetectionInfo> DetectionMethod2(const Ref<LowLevelILFunction>& ssa, uint32_t returnReg)
{
	std::vector<WinMainDetectionInfo> results;

	auto function = ssa->GetFunction();
	if (!function)
		return results;

	auto view = function->GetView();
	if (!view)
		return results;

	auto argc = view->GetSymbolByRawName("__p___argc");
	if (!argc)
		return results;

	auto sites = view->GetCodeReferences(argc->GetAddress());
	for (const auto& site: sites)
	{
		if ((!site.arch) || (!site.func))
			continue;
		auto llil = site.func->GetLowLevelIL();
		if (!llil)
			continue;

		auto index = site.func->GetLowLevelILForInstruction(site.arch, site.addr);
		auto il = llil->GetInstruction(index);
		if (il.operation != LLIL_CALL)
			continue;

		auto block = llil->GetBasicBlockForInstruction(il.instructionIndex);
		if (!block)
			continue;

		for (size_t i = il.instructionIndex + 1; i < block->GetEnd(); i++)
		{
			il = llil->GetInstruction(i);
			if (il.operation != LLIL_CALL)
				continue;
			auto dest = il.GetDestExpr<LLIL_CALL>();
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
		}

	}
	return results;
}


std::vector<Ref<Function>> GetFunctionsByName(const Ref<BinaryView>& view, const std::string& name)
{
	std::vector<Ref<Function>> results;

	std::set<uint64_t> addresses;
	auto syms = view->GetSymbolsByRawName(name);
	for (const auto& sym: syms)
	{
		auto symType = sym->GetType();
		if ((symType != FunctionSymbol) && (symType != ImportedFunctionSymbol) && (symType != LibraryFunctionSymbol))
			continue;
		addresses.emplace(sym->GetAddress());
	}

	for (const auto addr: addresses)
	{
		for (const auto& func: view->GetAnalysisFunctionsForAddress(addr))
			results.emplace_back(func);
	}

	return results;
}


std::vector<WinMainDetectionInfo> DetectionMethod3(const Ref<BinaryView>& view)
{
	std::vector<WinMainDetectionInfo> results;

	auto functions = GetFunctionsByName(view, "invoke_main");
	if (functions.size() != 1)
		return results;

	auto func = functions[0];
	auto llil = func->GetLowLevelIL();
	if (!llil)
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
		auto mainStubllil = mainStub->GetLowLevelIL();
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

	auto ssa = il->GetSSAForm();
	if (!ssa)
	{
		result.reason = "No SSA available";
		return result;
	}

	auto callingConvention = func->GetCallingConvention();
	if (!callingConvention)
	{
		result.reason = "No calling convention available";
		return result;
	}

	auto returnReg = callingConvention->GetIntegerReturnValueRegister();

	std::vector<WinMainDetectionInfo> candidates;
	auto results1 = DetectionMethod1(ssa, returnReg);
	auto results2 = DetectionMethod2(ssa, returnReg);
	auto results3 = DetectionMethod3(view);

	if (func->GetStart() == 0x140001010)
		LogWarn("here");

	candidates.insert(candidates.end(), results1.begin(), results1.end());
	candidates.insert(candidates.end(), results2.begin(), results2.end());
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


//WinMainDetectionInfo FindWinMain(const Ref<BinaryView>& bv)
//{
//	WinMainDetectionInfo result;
//
//	auto entryFunc = bv->GetAnalysisEntryPoint();
//	if (!entryFunc)
//		return result;
//
//	auto callSites = entryFunc->GetCallSites();
//	if (callSites.size() > 2)
//	{
//		result.reason = "More entrypoint callees than expected";
//		return result;
//	}
//
//	std::set<uint64_t> calleeAddresses;
//	for (const auto& callSite: callSites)
//	{
//		for (auto addr: bv->GetCallees(callSite))
//			calleeAddresses.emplace(addr);
//	}
//
//	for (const uint64_t addr: calleeAddresses)
//	{
//		auto func = bv->GetAnalysisFunction(bv->GetDefaultPlatform(), addr);
//		if (!func)
//			continue;
//
//		auto funcSymbol = func->GetSymbol();
//		if (!funcSymbol)
//			continue;
//
//		auto funcName = funcSymbol->GetShortName();
//		// This relies on the presence of the PDB, which is not always reliable. We should check if the function
//		// writes to the security_cookie data variable.
//		if (StringEndsWith(funcName, "security_init_cookie"))
//			continue;
//
//		WinMainDetectionInfo ret = IsCommonMain(func);
//		if (ret.found)
//			return ret;
//		else
//		{
//			if (result.reason.empty())
//				result.reason = ret.reason;
//			else
//				result.reason += (string(", ") + ret.reason);
//		}
//	}
//	return result;
//}


//std::string GetMethodString(const WinMainDetectionInfo& result)
//{
//	std::string s;
//	if (result.method1)
//		s += "1";
//	if (result.method2)
//		s += ",2";
//	if (result.method3)
//		s += ",3";
//	return s;
//}


//void ProcessOneFile(const std::string& path)
//{
//	printf("file: %s: ", path.c_str());
//
//	// Do not use the PDB symbol info
//	Ref<Metadata> options = new Metadata(KeyValueDataType);
//	options->SetValueForKey("analysis.debugInfo.internal", new Metadata(false));
//
//	Ref<BinaryView> bv = BinaryNinja::Load(path, false, {}, options);
//	if (!bv)
//	{
//		printf("failed to get a binary view\n");
//		return;
//	}
//
////	LogWarn("start: 0x%llx", bv->GetStart());
//
//	auto platform = bv->GetDefaultPlatform();
//	if ((!platform) || (platform->GetName() != "windows-x86_64"))
//	{
//		printf("unexpected platform: %s\n", platform ? platform->GetName().c_str() : "None");
//		bv->GetFile()->Close();
//		return;
//	}
//
//	if (IsDLL(bv))
//	{
//		printf("Skipping DLL\n");
//		bv->GetFile()->Close();
//		return;
//	}
//
//	if (IsDriver(bv))
//	{
//		printf("Skipping driver\n");
//		bv->GetFile()->Close();
//		return;
//	}
//
//	bv->UpdateAnalysisAndWait();
//
//	auto result = FindWinMain(bv);
//	if (result.found)
//	{
//		printf("found main at 0x%llx, method: %s\n", result.address, GetMethodString(result).c_str());
//	}
//	else
//	{
//		printf("cannot find main, reason: %s\n", result.reason.c_str());
//	}
//
//	bv->GetFile()->Close();
//}


WinMainFunctionRecognizer::WinMainFunctionRecognizer(Ref<Platform> platform) : m_platform(platform)
{
	auto settings = Settings::Instance();
	m_mainFunctionNames = settings->Get<vector<string>>("ui.files.navigation.mainSymbols");
}


bool WinMainFunctionRecognizer::MainFunctionFound(BinaryNinja::BinaryView *view)
{
	auto data = view->QueryMetadata("__BN_main_function_address");
	if (data && data->IsUnsignedInteger())
		return true;
	return false;
}


bool WinMainFunctionRecognizer::RecognizeLowLevelIL(BinaryView* view, Function* func, LowLevelILFunction* il)
{
	// Make sure the function belongs to the desired platform. Platform specific function recognizers
	// are not a feature so this was registered for the architecture as a whole.
	if (func->GetPlatform() != m_platform)
		return false;

	// main function has been found, returning
	if (MainFunctionFound(view))
		return true;

	auto entryFunc = view->GetAnalysisEntryPoint();
	if (!entryFunc)
		return false;

	auto entryPoint = entryFunc->GetStart();

	bool isCalledByEntry = false;
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

	if (!isCalledByEntry)
		return false;

	// TODO: this needs to be run on every function, which is bad
	if (IsDLL(view) || IsDriver(view))
		return false;

	auto info = IsCommonMain(view, func, il);
	if (info.found)
	{
//		LogWarn("main function found in function: 0x%llx", func->GetStart());
		auto entry = new Metadata(info.address);
		view->StoreMetadata("__BN_main_function_address", entry, true);
		return true;
	}

	return false;
}
