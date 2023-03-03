#pragma once

#include <QtWidgets/QTreeView>
#include <QtCore/QSortFilterProxyModel>
#include <memory>
#include "sidebar.h"
#include "viewframe.h"
#include "filter.h"


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
	virtual void generateChildren() = 0;
	void updateChildIndices();

public:
	class TypeBrowserModel* model() const { return m_model; }
	std::optional<std::shared_ptr<TypeBrowserTreeNode>> parent() const;
	const std::vector<std::shared_ptr<TypeBrowserTreeNode>>& children();
	int indexOfChild(std::shared_ptr<TypeBrowserTreeNode> child) const;

	virtual std::string text() const = 0;
	virtual bool operator<(const TypeBrowserTreeNode& other) const = 0;
	virtual bool filter(const std::string& filter) const = 0;
	virtual void updateChildren(RemoveNodeCallback remove, UpdateNodeCallback update, InsertNodeCallback insert);
};


class BINARYNINJAUIAPI RootTreeNode : public TypeBrowserTreeNode
{
	std::shared_ptr<class BinaryViewTreeNode> m_viewNode;
	std::map<TypeLibraryRef, std::shared_ptr<class TypeLibraryTreeNode>> m_libraryNodes;
	std::shared_ptr<class PlatformTreeNode> m_platformNode;
public:
	RootTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent);
	virtual ~RootTreeNode() = default;

	virtual std::string text() const override;
	virtual bool operator<(const TypeBrowserTreeNode& other) const override;
	virtual bool filter(const std::string& filter) const override;

protected:
	virtual void generateChildren() override;
	virtual void updateChildren(RemoveNodeCallback remove, UpdateNodeCallback update, InsertNodeCallback insert) override;
};


class BINARYNINJAUIAPI TypeTreeNode : public TypeBrowserTreeNode
{
	BinaryNinja::QualifiedName m_name;
	TypeRef m_type;

public:
	TypeTreeNode(class TypeBrowserModel* model, std::optional<std::weak_ptr<TypeBrowserTreeNode>> parent, BinaryNinja::QualifiedName name, TypeRef type);
	virtual ~TypeTreeNode() = default;

	const BinaryNinja::QualifiedName& name() const { return m_name; }
	const TypeRef& type() const { return m_type; }
	void setType(const TypeRef& type) { m_type = type; }

	virtual std::string text() const override;
	virtual bool operator<(const TypeBrowserTreeNode& other) const override;
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

	virtual std::string text() const override;
	virtual bool operator<(const TypeBrowserTreeNode& other) const override;
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

	virtual std::string text() const override;
	virtual bool operator<(const TypeBrowserTreeNode& other) const override;
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

	virtual std::string text() const override;
	virtual bool operator<(const TypeBrowserTreeNode& other) const override;
	virtual bool filter(const std::string& filter) const override;

	virtual std::map<BinaryNinja::QualifiedName, TypeRef> getTypes() const override;
};


class BINARYNINJAUIAPI TypeBrowserModel : public QAbstractItemModel, public BinaryNinja::BinaryDataNotification
{
	Q_OBJECT
	BinaryViewRef m_data;
	std::shared_ptr<TypeBrowserTreeNode> m_rootNode;
	bool m_needsUpdate;

public:
	TypeBrowserModel(BinaryViewRef data);
	virtual ~TypeBrowserModel();
	BinaryViewRef getData() { return m_data; }

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
public Q_SLOTS:
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


class BINARYNINJAUIAPI TypeBrowserView : public QFrame, public View, public FilterTarget
{
	Q_OBJECT
	ViewFrame* m_frame;
	BinaryViewRef m_data;
	class TypeBrowserContainer* m_container;

	TypeBrowserModel* m_model;
	TypeBrowserFilterModel* m_filterModel;
	QTreeView* m_tree;

public:
	TypeBrowserView(ViewFrame* frame, BinaryViewRef data, TypeBrowserContainer* container);

	virtual BinaryViewRef getData() override { return m_data; }
	virtual uint64_t getCurrentOffset() override;
	virtual void setSelectionOffsets(BNAddressRange range) override;
	virtual bool navigate(uint64_t offset) override;
	virtual QFont getFont() override;
	virtual void updateFonts() override;

	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;

	virtual StatusBarWidget* getStatusBarWidget() override;

	virtual void setFilter(const std::string& filter) override;
	virtual void scrollToFirstItem() override;
	virtual void scrollToCurrentItem() override;
	virtual void selectFirstItem() override;
	virtual void activateFirstItem() override;

	virtual void notifyRefresh() override;

protected:
	void itemSelected(const QModelIndex& index);
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

	TypeBrowserView* getTypeBrowserView() { return m_view; }
	FilteredView* getFilter() { return m_filter; }
	FilterEdit* getSeparateFilterEdit() { return m_separateEdit; }

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