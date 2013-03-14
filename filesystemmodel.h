#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>
#include <QVector>

class QFileIconProvider;
class QFileInfo;

class FilesystemModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit FilesystemModel(QObject *parent = 0);
	~FilesystemModel();

	QModelIndex index(int row, int column, const QModelIndex &parent) const;
	QModelIndex parent(const QModelIndex &child) const;

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	QVariant data(const QModelIndex &index, int role) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	bool canFetchMore(const QModelIndex &parent) const;
	void fetchMore(const QModelIndex &parent);

	Qt::ItemFlags flags(const QModelIndex &index) const;

	bool hasChildren(const QModelIndex &parent) const;

private:
	enum Columns
	{
		RamificationColumn,
		NameColumn = RamificationColumn,
		ModificationDateColumn,
		SizeColumn,
		TypeColumn,
		ColumnCount
	};

	struct NodeInfo;
	typedef QVector<NodeInfo> NodeInfoList;
	NodeInfoList _nodes;
	QScopedPointer<QFileIconProvider> _metaProvider;

	void fetchRootDirectory();
	int findRow(const NodeInfo* nodeInfo) const;
	QVariant nameData(const QFileInfo& fileInfo, int role) const;

};

#endif // FILESYSTEMMODEL_H
