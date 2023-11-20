#pragma once
#include "binaryninjaapi.h"

struct WinMainDetectionInfo
{
	bool found = false;
	bool method1 = false;
	bool method2 = false;
	bool method3 = false;
	uint64_t address = 0;
	std::string reason;
};


class WinMainFunctionRecognizer : public BinaryNinja::FunctionRecognizer
{
	BinaryNinja::Ref<BinaryNinja::Platform> m_platform;
	std::vector<std::string> m_mainFunctionNames;
	std::vector<std::string> m_exitFunctionNames;

	WinMainDetectionInfo IsCommonMain(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
									  BinaryNinja::LowLevelILFunction* il);
	// This method checks whether the return value of a function call sinks to exit. If so, the function is considered
	// a main candidate. Note this method is purely based on SSA dataflow and does not rely on any symbol information.
	// in most cases it is the detection method that actually finds the main. Note, the main function detection runs
	// very early and the PDB symbol info could still be loading when it starts.
	std::vector<WinMainDetectionInfo> FindMainViaReturnSinkToExit(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
													   BinaryNinja::LowLevelILFunction* il);
	// This method finds an instruction that calls __p__argc and find the next call instruction after it. If the
	// target of the next call has no symbol or has a main symbol, it is considered a main candidate.
	std::vector<WinMainDetectionInfo> FindMainViaCommandLineArgUsage(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
													   BinaryNinja::LowLevelILFunction* il);
	// This method finds a function called invoke_main and finds a call to the one-instruction stub that calls the
	// main function in it.
	std::vector<WinMainDetectionInfo> FindMainViaInvokeMain(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
													   BinaryNinja::LowLevelILFunction* il);

	bool IsExitFunction(BinaryNinja::BinaryView* view, uint64_t address);
	std::optional<BinaryNinja::SSARegister> GetRegisterSetInBlocks(BinaryNinja::BasicBlock* block, uint32_t reg,
													  uint64_t maxSearchIndex = -1);
	// This function collects list of the ssa registers whose value are either passed as the first parameter to an exit
	// function, or used as the return value of the function. The result of this function is passed into SinkToReturn
	std::set<BinaryNinja::SSARegister> GetTargetSSARegisters(BinaryNinja::LowLevelILFunction* ssa);
	// This function checks whether the value of the queried ssa reg sinks to a set of ssa regs. This function is used
	// in detection methods 1
	bool SinkToReturn(const std::set<BinaryNinja::SSARegister>& targetRegs, BinaryNinja::LowLevelILFunction* func,
					  const BinaryNinja::SSARegister& ssaReg, std::set<uint32_t>& seen);

	bool MainFunctionDetectionDone(BinaryNinja::BinaryView* view);

public:
	WinMainFunctionRecognizer(BinaryNinja::Ref<BinaryNinja::Platform> platform);
	bool RecognizeLowLevelIL(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
							 BinaryNinja::LowLevelILFunction* il) override;
};
