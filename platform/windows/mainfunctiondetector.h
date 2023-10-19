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
	std::vector<WinMainDetectionInfo> DetectionMethod1(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
													   BinaryNinja::LowLevelILFunction* il);
	std::vector<WinMainDetectionInfo> DetectionMethod2(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
													   BinaryNinja::LowLevelILFunction* il);
	std::vector<WinMainDetectionInfo> DetectionMethod3(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
													   BinaryNinja::LowLevelILFunction* il);

	bool IsExitFunction(BinaryNinja::BinaryView* view, uint64_t address);
	std::optional<BinaryNinja::SSARegister> GetRegisterSetInBlocks(BinaryNinja::BasicBlock* block, uint32_t reg,
													  uint64_t maxSearchIndex = -1);
	std::set<BinaryNinja::SSARegister> GetTargetSSARegisters(BinaryNinja::LowLevelILFunction* ssa);
	bool SinkToReturn(const std::set<BinaryNinja::SSARegister>& targetRegs, BinaryNinja::LowLevelILFunction* func,
					  const BinaryNinja::SSARegister& ssaReg, std::set<uint32_t>& seen);

	bool MainFunctionDetectionDone(BinaryNinja::BinaryView* view);

public:
	WinMainFunctionRecognizer(BinaryNinja::Ref<BinaryNinja::Platform> platform);
	bool RecognizeLowLevelIL(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
							 BinaryNinja::LowLevelILFunction* il) override;
};
