#pragma once

#include <QtWidgets/QTreeView>
#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QItemDelegate>
#include <QtWidgets/QTextEdit>
#include <memory>
#include "sidebar.h"
#include "viewframe.h"
#include "filter.h"
#include "progresstask.h"


class BINARYNINJAUIAPI TypeBrowserTreeNode : public std::enable_shared_from_this<TypeBrowserTreeNode>
{
public:
	typedef std::function<void(std::shared_ptr<TypeBrowserTreeNode>, std::function<void()>)> RemoveNodeCallback;
	typedef std::function<void(std::shared_ptr<TypeBrowserTreeNode>, std::function<void()>)> UpdateNodeCallback;
	typedef std::function<void(std::shared_ptr<TypeBrowserTreeNode>, int, std::function<void()>)> InsertNodeCallback;

protected:
	class TypeBrowserModel* m_model;
	std::optional<std::weak_ptr<TypeBrowserTreeNode>> m_parent;
	std::vector<std::shared_ptr<TypeBrowserTreeNode>> m_children;
	std::map<TypeBrowserTreeNode*, size_t> m_childIndices;
	bool m_hasGeneratedChildren;

	TypeBrowserTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent);
	virtual ~TypeBrowserTreeNode() = default;
	void ensureGeneratedChildren();
	virtual void generateChildren() = 0;
	void updateChildIndices();

	template<typename T, typename T2>
	friend void updateNodes(TypeBrowserTreeNode* node, std::map<T, std::shared_ptr<T2>>& nodes, const std::vector<T>& newList, TypeBrowserTreeNode::RemoveNodeCallback remove, TypeBrowserTreeNode::UpdateNodeCallback update, TypeBrowserTreeNode::InsertNodeCallback insert);

public:
	class TypeBrowserModel* model() const { return m_model; }
	std::optional<std::shared_ptr<TypeBrowserTreeNode>> parent() const;
	const std::vector<std::shared_ptr<TypeBrowserTreeNode>>& children();
	int indexOfChild(std::shared_ptr<TypeBrowserTreeNode> child) const;

	virtual std::string text(int column) const = 0;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const = 0;
	virtual bool filter(const std::string& filter) const = 0;
	virtual void updateChildren(RemoveNodeCallback remove, UpdateNodeCallback update, InsertNodeCallback insert);
};


class BINARYNINJAUIAPI EmptyTreeNode : public TypeBrowserTreeNode
{
public:
	EmptyTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent);
	virtual ~EmptyTreeNode() = default;

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

protected:
	virtual void generateChildren() override;
	virtual void updateChildren(RemoveNodeCallback remove, UpdateNodeCallback update, InsertNodeCallback insert) override;
};


class BINARYNINJAUIAPI RootTreeNode : public TypeBrowserTreeNode
{
	std::shared_ptr<class BinaryViewTreeNode> m_viewNode;
	std::map<std::string, std::shared_ptr<class TypeArchiveTreeNode>> m_archiveNodes;
	std::map<TypeLibraryRef, std::shared_ptr<class TypeLibraryTreeNode>> m_libraryNodes;
	std::map<std::string, std::shared_ptr<class DebugInfoTreeNode>> m_debugInfoNodes;
	std::shared_ptr<class PlatformTreeNode> m_platformNode;
public:
	RootTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent);
	virtual ~RootTreeNode() = default;

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

protected:
	virtual void generateChildren() override;
	virtual void updateChildren(RemoveNodeCallback remove, UpdateNodeCallback update, InsertNodeCallback insert) override;
};


class BINARYNINJAUIAPI TypeTreeNode : public TypeBrowserTreeNode
{
public:
	enum SourceType
	{
		None,
		TypeLibrary,
		TypeArchive,
		DebugInfo,
		Platform,
		Other
	};

private:
	BinaryNinja::QualifiedName m_name;
	TypeRef m_type;

	SourceType m_sourceType;
	std::optional<TypeLibraryRef> m_sourceLibrary;
	std::optional<TypeArchiveRef> m_sourceArchive;
	std::optional<std::string> m_sourceDebugInfoParser;
	std::optional<PlatformRef> m_sourcePlatform;
	std::optional<std::string> m_sourceOtherName;
	std::optional<BinaryNinja::QualifiedName> m_sourceOriginalName;

public:
	TypeTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, BinaryNinja::QualifiedName name, TypeRef type);
	virtual ~TypeTreeNode() = default;

	const BinaryNinja::QualifiedName& name() const { return m_name; }
	const TypeRef& type() const { return m_type; }
	void setType(const TypeRef& type) { m_type = type; }

	const SourceType& sourceType() const { return m_sourceType; }
	const std::optional<TypeLibraryRef>& sourceLibrary() const { return m_sourceLibrary; }
	const std::optional<TypeArchiveRef>& sourceArchive() const { return m_sourceArchive; }
	const std::optional<std::string>& sourceDebugInfoParser() const { return m_sourceDebugInfoParser; }
	const std::optional<PlatformRef>& sourcePlatform() const { return m_sourcePlatform; }
	const std::optional<std::string>& sourceOtherName() const { return m_sourceOtherName; }
	const std::optional<BinaryNinja::QualifiedName>& sourceOriginalName() const { return m_sourceOriginalName; }

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

protected:
	virtual void generateChildren() override;
};


class BINARYNINJAUIAPI TypeSourceTreeNode : public TypeBrowserTreeNode
{
	std::map<BinaryNinja::QualifiedName, std::pair<TypeRef, std::shared_ptr<TypeTreeNode>>> m_typeNodes;

public:
	TypeSourceTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent);
	virtual ~TypeSourceTreeNode();

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const = 0;
	virtual void updateChildren(RemoveNodeCallback remove, UpdateNodeCallback update, InsertNodeCallback insert) override;

protected:
	virtual void generateChildren() override;
};


class BINARYNINJAUIAPI BinaryViewTreeNode : public TypeSourceTreeNode
{
	BinaryViewRef m_view;

public:
	BinaryViewTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, BinaryViewRef view);
	virtual ~BinaryViewTreeNode() = default;

	const BinaryViewRef& view() const { return m_view; }

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const override;
};


class BINARYNINJAUIAPI TypeArchiveTreeNode : public TypeSourceTreeNode
{
	std::string m_archiveId;
	std::optional<std::string> m_archivePath;
	TypeArchiveRef m_archive;

public:
	TypeArchiveTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, const std::string& archiveId);
	virtual ~TypeArchiveTreeNode() = default;

	const std::string& archiveId() const { return m_archiveId; }
	const TypeArchiveRef& archive() const { return m_archive; }

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const override;
};


class BINARYNINJAUIAPI TypeLibraryTreeNode : public TypeSourceTreeNode
{
	TypeLibraryRef m_library;

public:
	TypeLibraryTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, TypeLibraryRef library);
	virtual ~TypeLibraryTreeNode() = default;

	const TypeLibraryRef& library() const { return m_library; }

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const override;
};


class BINARYNINJAUIAPI DebugInfoTreeNode : public TypeSourceTreeNode
{
	DebugInfoRef m_debugInfo;
	std::string m_parserName;

public:
	DebugInfoTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, const std::string& parserName);
	virtual ~DebugInfoTreeNode() = default;

	const DebugInfoRef& debugInfo() const { return m_debugInfo; }
	const std::string& parserName() const { return m_parserName; }

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const override;
};


class BINARYNINJAUIAPI PlatformTreeNode : public TypeSourceTreeNode
{
	PlatformRef m_platform;

public:
	PlatformTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, PlatformRef platform);
	virtual ~PlatformTreeNode() = default;

	const PlatformRef& platform() const { return m_platform; }

	virtual std::string text(int column) const override;
	virtual bool lessThan(const TypeBrowserTreeNode& other, int column) const override;
	virtual bool filter(const std::string& filter) const override;

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const override;
};


//-----------------------------------------------------------------------------


class BINARYNINJAUIAPI TypeBrowserModel : public QAbstractItemModel, public BinaryNinja::BinaryDataNotification, public BinaryNinja::TypeArchiveNotification
{
	Q_OBJECT
	BinaryViewRef m_data;
	std::shared_ptr<TypeBrowserTreeNode> m_rootNode;
	mutable std::recursive_mutex m_rootNodeMutex;
	bool m_needsUpdate;

public:
	TypeBrowserModel(BinaryViewRef data);
	virtual ~TypeBrowserModel();
	BinaryViewRef getData() { return m_data; }
	std::shared_ptr<TypeBrowserTreeNode> getRootNode() { return m_rootNode; }

	void startUpdate();

	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	std::shared_ptr<TypeBrowserTreeNode> nodeForIndex(const QModelIndex& index) const;
	QModelIndex indexForNode(std::shared_ptr<TypeBrowserTreeNode> node, int column = 0) const;

	bool filter(const QModelIndex& index, const std::string& filter) const;
	bool lessThan(const QModelIndex& left, const QModelIndex& right) const;

	void OnTypeDefined(BinaryNinja::BinaryView *data, const BinaryNinja::QualifiedName &name, BinaryNinja::Type *type) override;
	void OnTypeUndefined(BinaryNinja::BinaryView *data, const BinaryNinja::QualifiedName &name, BinaryNinja::Type *type) override;
	void OnTypeReferenceChanged(BinaryNinja::BinaryView *data, const BinaryNinja::QualifiedName &name, BinaryNinja::Type *type) override;
	void OnTypeFieldReferenceChanged(BinaryNinja::BinaryView *data, const BinaryNinja::QualifiedName &name, uint64_t offset) override;

	void OnTypeAdded(TypeArchiveRef archive, const std::string& id, TypeRef definition) override;
	void OnTypeUpdated(TypeArchiveRef archive, const std::string& id, TypeRef oldDefinition, TypeRef newDefinition) override;
	void OnTypeRenamed(TypeArchiveRef archive, const std::string& id, const BinaryNinja::QualifiedName& oldName, const BinaryNinja::QualifiedName& newName) override;
	void OnTypeDeleted(TypeArchiveRef archive, const std::string& id, TypeRef definition) override;

	void OnTypeArchiveAttached(BinaryNinja::BinaryView* data, const std::string& id, const std::string& path) override;
	void OnTypeArchiveDetached(BinaryNinja::BinaryView* data, const std::string& id, const std::string& path) override;
	void OnTypeArchiveConnected(BinaryNinja::BinaryView* data, BinaryNinja::TypeArchive* archive) override;
	void OnTypeArchiveDisconnected(BinaryNinja::BinaryView* data, BinaryNinja::TypeArchive* archive) override;

public Q_SLOTS:
	void markDirty();
	void notifyRefresh();
};


class BINARYNINJAUIAPI TypeBrowserFilterModel : public QSortFilterProxyModel
{
	Q_OBJECT
	BinaryViewRef m_data;
	TypeBrowserModel* m_model;
	std::string m_filter;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

public:
	TypeBrowserFilterModel(BinaryViewRef data, TypeBrowserModel* model);

	void setFilter(const std::string& filter);
};


class BINARYNINJAUIAPI TypeBrowserItemDelegate : public QItemDelegate
{
	QFont m_font;
	QFont m_monospaceFont;
	int m_baseline, m_charWidth, m_charHeight, m_charOffset;
	class TypeBrowserView* m_view;

public:
	TypeBrowserItemDelegate(class TypeBrowserView* view);
	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
	virtual void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};


class BINARYNINJAUIAPI TypeBrowserView : public QFrame, public View, public FilterTarget
{
	Q_OBJECT
	ViewFrame* m_frame;
	BinaryViewRef m_data;
	class TypeBrowserContainer* m_container;
	ContextMenuManager* m_contextMenuManager;

	QSplitter* m_splitter;

	TypeBrowserModel* m_model;
	TypeBrowserFilterModel* m_filterModel;
	QStandardItemModel* m_loadingModel;
	QTreeView* m_tree;
	TypeBrowserItemDelegate* m_delegate;
	bool m_updatedWidths;

	class TypesContainer* m_typeEditor;

	QWidget* m_plaintextTypeEditor;
	QTextEdit* m_typeEditorDefinition;
	QTextEdit* m_typeEditorArguments;
	QTextEdit* m_typeEditorErrors;

	QTimer* m_filterTimer;

public:
	TypeBrowserView(ViewFrame* frame, BinaryViewRef data, TypeBrowserContainer* container);

	TypeBrowserContainer* getContainer() { return m_container; }
	TypeBrowserModel* getModel() { return m_model; }
	TypeBrowserFilterModel* getFilterModel() { return m_filterModel; }

	virtual BinaryViewRef getData() override { return m_data; }
	virtual uint64_t getCurrentOffset() override;
	virtual void setSelectionOffsets(BNAddressRange range) override;
	virtual bool navigate(uint64_t offset) override;
	virtual QFont getFont() override;
	virtual void updateFonts() override;

	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;

	virtual StatusBarWidget* getStatusBarWidget() override;
	virtual QWidget* getHeaderOptionsWidget() override;

	virtual void setFilter(const std::string& filter) override;
	virtual void scrollToFirstItem() override;
	virtual void scrollToCurrentItem() override;
	virtual void selectFirstItem() override;
	virtual void activateFirstItem() override;

	virtual void notifyRefresh() override;

	std::vector<std::shared_ptr<TypeBrowserTreeNode>> selectedNodes() const;

	// Menu actions
	static void registerActions();
	void bindActions();
	void showContextMenu();

	bool canConnectTypeArchive();
	void connectTypeArchive();

	bool canCreateTypeArchive();
	void createTypeArchive();
	bool canAttachTypeArchive();
	void attachTypeArchive();
	bool canDetachTypeArchive();
	void detachTypeArchive();

	bool canSyncSelectedTypes();
	void syncSelectedTypes();
	bool canPushSelectedTypes();
	void pushSelectedTypes();
	bool canPullSelectedTypes();
	void pullSelectedTypes();
	bool canRevertSelectedTypes();
	void revertSelectedTypes();
	bool canDisassociateSelectedTypes();
	void disassociateSelectedTypes();

	bool canCreateNewTypes();
	void createNewTypes();
	bool canRenameTypes();
	void renameTypes();
	bool canDeleteTypes();
	void deleteTypes();
	bool canChangeTypes();
	void changeTypes();

protected:
	void itemSelected(const QItemSelection& selected, const QItemSelection& deselected);
	void itemDoubleClicked(const QModelIndex& index);
	virtual void contextMenuEvent(QContextMenuEvent* event) override;
};

class BINARYNINJAUIAPI TypeBrowserOptionsIconWidget : public QWidget
{
public:
	TypeBrowserOptionsIconWidget(TypeBrowserView* parent);

private:
	TypeBrowserView* m_view;

	void showMenu();
};

class BINARYNINJAUIAPI TypeBrowserContainer : public QWidget, public ViewContainer
{
	Q_OBJECT

	ViewFrame* m_frame;
	BinaryViewRef m_data;
	TypeBrowserView* m_view;
	FilteredView* m_filter;
	FilterEdit* m_separateEdit;
	class TypeBrowserSidebarWidget* m_sidebarWidget;

public:
	TypeBrowserContainer(ViewFrame* frame, BinaryViewRef data, class TypeBrowserSidebarWidget* parent);
	virtual View* getView() override { return m_view; }

	ViewFrame* getViewFrame() { return m_frame; }
	BinaryViewRef getData() { return m_data; }
	TypeBrowserView* getTypeBrowserView() { return m_view; }
	FilteredView* getFilter() { return m_filter; }
	FilterEdit* getSeparateFilterEdit() { return m_separateEdit; }
	class TypeBrowserSidebarWidget* getSidebarWidget() { return m_sidebarWidget; }
	void showContextMenu();

protected:
	virtual void focusInEvent(QFocusEvent* event) override;
};


class BINARYNINJAUIAPI TypeBrowserViewType : public ViewType
{
	static TypeBrowserViewType* g_instance;

public:
	TypeBrowserViewType();
	virtual int getPriority(BinaryViewRef data, const QString& filename) override;
	virtual QWidget* create(BinaryViewRef data, ViewFrame* frame) override;
	static void init();
};


class BINARYNINJAUIAPI TypeBrowserSidebarWidget : public SidebarWidget
{
	Q_OBJECT

	QWidget* m_header;
	TypeBrowserContainer* m_container;

public:
	TypeBrowserSidebarWidget(ViewFrame* frame, BinaryViewRef data);
	virtual QWidget* headerWidget() override { return m_header; }
	virtual void focus() override;

protected:
	virtual void contextMenuEvent(QContextMenuEvent* event) override;

private Q_SLOTS:
	void showContextMenu();
};


class BINARYNINJAUIAPI TypeBrowserSidebarWidgetType : public SidebarWidgetType
{
public:
	TypeBrowserSidebarWidgetType();
	virtual SidebarWidget* createWidget(ViewFrame* frame, BinaryViewRef data) override;
};
