
#include "obs-module.h"
#include <QDialog>
#include <QThread>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <util/source-profiler.h>

class PerfTreeModel;
class PerfViewerProxyModel;

class OBSPerfViewer : public QDialog {
	Q_OBJECT

	PerfTreeModel *model = nullptr;
	PerfViewerProxyModel *proxy = nullptr;

	QTreeView *treeView = nullptr;

	bool loaded = false;

public:
	OBSPerfViewer(QWidget *parent = nullptr);
	~OBSPerfViewer() override;

public slots:
	void sourceListUpdated();
	void sortColumn(int index);
};

class PerfTreeItem;

class PerfTreeModel : public QAbstractItemModel {
	Q_OBJECT

public:
	explicit PerfTreeModel(QObject *parent = nullptr);
	~PerfTreeModel() override;

	QVariant data(const QModelIndex &index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	void itemChanged(PerfTreeItem *item);

	enum ShowMode { SCENE, SOURCE, FILTER, TRANSITION, ALL };

	void setShowMode(enum ShowMode s = ShowMode::SCENE)
	{
		showMode = s;
		refreshSources();
	}
	enum ShowMode getShowMode() { return showMode; }

	void setRefreshInterval(int interval);

	double targetFrameTime() const { return frameTime; }

	enum Columns {
		NAME,
		TYPE,
		ACTIVE,
		TICK,
		RENDER,
#ifndef __APPLE__
		RENDER_GPU,
#endif
		FPS,
		FPS_RENDERED,
		NUM_COLUMNS
	};

	enum PerfRole {
		SortRole = Qt::UserRole,
		RawDataRole,
	};

public slots:
	void refreshSources();

private slots:
	void updateData();

private:
	PerfTreeItem *rootItem = nullptr;
	QList<QVariant> header;
	std::unique_ptr<QThread> updater;

	enum ShowMode showMode = ShowMode::SCENE;
	bool refreshing = false;
	double frameTime = 0.0;
	int refreshInterval = 1000;
};

class PerfTreeItem {
public:
	explicit PerfTreeItem(obs_source_t *source, PerfTreeItem *parentItem = nullptr, PerfTreeModel *model = nullptr);
	explicit PerfTreeItem(QString name, PerfTreeItem *parentItem = nullptr, PerfTreeModel *model = nullptr);
	~PerfTreeItem();

	void appendChild(PerfTreeItem *item);
	void prependChild(PerfTreeItem *item);

	PerfTreeItem *child(int row) const;
	int childCount() const;
	int columnCount() const;
	QVariant data(int column) const;
	QVariant rawData(int column) const;
	QVariant sortData(int column) const;
	int row() const;
	PerfTreeItem *parentItem();

	PerfTreeModel *model() const { return m_model; }

	void update();
	QIcon getIcon() const;
	QVariant getTextColour(int column) const;

	bool isRendered() const { return rendered; }

private:
	QList<PerfTreeItem *> m_childItems;
	PerfTreeItem *m_parentItem;
	PerfTreeModel *m_model;

	profiler_result_t *m_perf = nullptr;
	obs_source_t *m_source = nullptr;
	QString name;
	bool rendered = false;
};

class PerfViewerProxyModel : public QSortFilterProxyModel {
	Q_OBJECT

public:
	PerfViewerProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent)
	{
		setRecursiveFilteringEnabled(true);
		setSortRole(PerfTreeModel::SortRole);
	}

public slots:
	void setFilterText(const QString &filter);

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class QuickThread : public QThread {
public:
	explicit inline QuickThread(std::function<void()> func_) : func(func_) {}

private:
	virtual void run() override { func(); }

	std::function<void()> func;
};
