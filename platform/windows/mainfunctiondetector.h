#pragma once

#include "binaryninjaapi.h"

class WinMainFunctionRecognizer : public BinaryNinja::FunctionRecognizer
{
	BinaryNinja::Ref<BinaryNinja::Platform> m_platform;
	std::vector<std::string> m_mainFunctionNames;
	std::vector<std::string> m_exitFunctionNames;

	bool MainFunctionDetectionDone(BinaryNinja::BinaryView* view);

public:
	WinMainFunctionRecognizer(BinaryNinja::Ref<BinaryNinja::Platform> platform);
	bool RecognizeLowLevelIL(BinaryNinja::BinaryView* view, BinaryNinja::Function* func,
							 BinaryNinja::LowLevelILFunction* il) override;
};
