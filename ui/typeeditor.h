
#pragma once

#include "tokenizedtextwidget.h"
#include "uitypes.h"

class BINARYNINJAUIAPI TypeEditor: public TokenizedTextWidget
{
	Q_OBJECT

	PlatformRef m_platform;
	std::optional<BinaryNinja::TypeContainer> m_typeContainer;
	std::vector<BinaryNinja::QualifiedName> m_typeNames;

	std::vector<BinaryNinja::QualifiedName> m_lineTypeRefs;	std::vector<BinaryNinja::TypeDefinitionLine> m_typeLines;

	TokenizedTextWidgetCursorPosition m_originalBase;

public:
	TypeEditor(QWidget* parent);

	void bindActions();

	PlatformRef platform() const { return m_platform; }
	void setPlatform(PlatformRef platform) { m_platform = platform; }

	std::optional<std::reference_wrapper<const BinaryNinja::TypeContainer>> typeContainer() const;
	void setTypeContainer(std::optional<BinaryNinja::TypeContainer> container);

	std::vector<BinaryNinja::QualifiedName> typeNames() const { return m_typeNames; }
	void setTypeNames(const std::vector<BinaryNinja::QualifiedName>& names);

	std::optional<std::reference_wrapper<const BinaryNinja::TypeDefinitionLine>> typeLineAtPosition(const TokenizedTextWidgetCursorPosition& position) const;

	bool canCreateAllMembersForStructure();
	void createAllMembersForStructure();
	bool canCreateCurrentMemberForStructure();
	void createCurrentMemberForStructure();
	bool canDefineName();
	void defineName();
	bool canUndefine();
	void undefine();
	bool canCreateArray();
	void createArray();
	bool canChangeType();
	void changeType();
	bool canSetStructureSize();
	void setStructureSize();
	bool canAddUserXref();
	void addUserXref();
	bool canMakePointer();
	void makePointer();
	bool canMakeCString();
	void makeCString();
	bool canMakeUTF16String();
	void makeUTF16String();
	bool canMakeUTF32String();
	void makeUTF32String();
	bool canCycleIntegerSize();
	void cycleIntegerSize();
	bool canCycleFloatSize();
	void cycleFloatSize();
	bool canInvertIntegerSize();
	void invertIntegerSize();
	bool canMakeInt8();
	void makeInt8();
	bool canMakeInt16();
	void makeInt16();
	bool canMakeInt32();
	void makeInt32();
	bool canMakeInt64();
	void makeInt64();
	bool canMakeFloat32();
	void makeFloat32();
	bool canMakeFloat64();
	void makeFloat64();
	bool canGoToAddress(bool selecting);
	void goToAddress(bool selecting);

Q_SIGNALS:
	void typeNameNavigated(const std::string& typeName);

private:
	void updateLines();
};
