
#pragma once

#include "tokenizedtextwidget.h"
#include "uitypes.h"

class BINARYNINJAUIAPI TypeEditor: public TokenizedTextWidget
{
	Q_OBJECT

	PlatformRef m_platform;
	std::optional<BinaryNinja::TypeContainer> m_typeContainer;
	std::optional<BinaryViewRef> m_binaryView;
	// Empty view for bv-requiring operations
	mutable std::optional<BinaryViewRef> m_emptyView;
	std::vector<BinaryNinja::QualifiedName> m_typeNames;

	// line index -> type name
	std::vector<BinaryNinja::QualifiedName> m_lineTypeRefs;
	// type name -> index of first line
	std::map<BinaryNinja::QualifiedName, size_t> m_lineTypeStarts;
	// type name -> { line index -> offset }
	std::map<BinaryNinja::QualifiedName, std::map<size_t, size_t>> m_lineTypeOffsets;
	// type name -> { offset -> index of first line }
	std::map<BinaryNinja::QualifiedName, std::map<size_t, size_t>> m_lineTypeOffsetStarts;
	// line index -> line
	std::vector<BinaryNinja::TypeDefinitionLine> m_typeLines;

	TokenizedTextWidgetCursorPosition m_originalBase;

	bool m_wrapLines;

public:
	TypeEditor(QWidget* parent);

	static void registerActions();
	void bindActions();

	PlatformRef platform() const { return m_platform; }
	void setPlatform(PlatformRef platform) { m_platform = platform; }

	std::optional<BinaryViewRef> binaryView() const { return m_binaryView; }
	void setBinaryView(std::optional<BinaryViewRef> binaryView) { m_binaryView = binaryView; }

	std::optional<std::reference_wrapper<const BinaryNinja::TypeContainer>> typeContainer() const;
	void setTypeContainer(std::optional<BinaryNinja::TypeContainer> container);

	std::vector<BinaryNinja::QualifiedName> typeNames() const { return m_typeNames; }
	void setTypeNames(const std::vector<BinaryNinja::QualifiedName>& names);

	std::optional<std::reference_wrapper<const BinaryNinja::TypeDefinitionLine>> typeLineAtPosition(const TokenizedTextWidgetCursorPosition& position) const;
	std::optional<BinaryNinja::QualifiedName> rootTypeNameAtPosition(const TokenizedTextWidgetCursorPosition& position) const;
	std::optional<TypeRef> rootTypeAtPosition(const TokenizedTextWidgetCursorPosition& position) const;
	std::optional<uint64_t> offsetAtPosition(const TokenizedTextWidgetCursorPosition& position) const;

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
	void toggleWrapLines();

Q_SIGNALS:
	void typeNameNavigated(const std::string& typeName);
	void currentTypeUpdated(const BinaryNinja::QualifiedName& typeName);
	void currentTypeDeleted(const BinaryNinja::QualifiedName& typeName);
	void currentTypeNameUpdated(const BinaryNinja::QualifiedName& typeName);

private:
	void updateLines();
	BinaryViewRef binaryViewOrEmpty() const;
	void updateInTransaction(std::function<void()> transaction);
	void updateInTransaction(std::function<void(BinaryViewRef)> transaction);
};
