
#pragma once

#include "tokenizedtextwidget.h"
#include "uitypes.h"

class BINARYNINJAUIAPI TypeEditor: public TokenizedTextWidget
{
	PlatformRef m_platform;
	std::optional<BinaryNinja::TypeContainer> m_typeContainer;
	std::vector<BinaryNinja::QualifiedName> m_typeNames;

	std::vector<BinaryNinja::QualifiedName> m_lineTypeRefs;

public:
	TypeEditor(QWidget* parent);

	void bindActions();

	PlatformRef platform() const { return m_platform; }
	void setPlatform(PlatformRef platform) { m_platform = platform; }

	std::optional<std::reference_wrapper<const BinaryNinja::TypeContainer>> typeContainer() const;
	void setTypeContainer(std::optional<BinaryNinja::TypeContainer> container);

	std::vector<BinaryNinja::QualifiedName> typeNames() const { return m_typeNames; }
	void setTypeNames(const std::vector<BinaryNinja::QualifiedName>& names);

public Q_SLOTS:
	void createTypes();
	void createStructure();
	void createUnion();
	void createAllMembersForStructure();
	void createCurrentMemberForStructure();
	void defineName();
	void undefine();
	void createArray();
	void changeType();
	void setStructureSize();
	void addUserXref();
	void makePointer();
	void makeCString();
	void makeUTF16String();
	void makeUTF32String();
	void cycleIntegerSize();
	void cycleFloatSize();
	void invertIntegerSize();
	void makeInt8();
	void makeInt16();
	void makeInt32();
	void makeInt64();
	void makeFloat32();
	void makeFloat64();

public:
	void goToAddress(bool selecting);

private:
	void updateLines();
};
