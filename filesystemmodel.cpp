#include "filesystemmodel.h"
#include <QFileInfo>
#include <QDir>
#include <algorithm>
#include <QFileIconProvider>
#include <QDateTime>
#include <QDebug>

FilesystemModel::FilesystemModel(QObject *parent) :
	QAbstractItemModel(parent),
	_metaProvider(new QFileIconProvider())
{
	fetchRootDirectory();
}

struct FilesystemModel::NodeInfo
{
	NodeInfo():
		parent(0),
		mapped(false)
	{}

	NodeInfo(const QFileInfo& fileInfo, NodeInfo* parent = 0):
		fileInfo(fileInfo),
		parent(parent),
		mapped(false)
	{}

	bool operator ==(const NodeInfo& another) const
	{
		bool r = this->fileInfo == another.fileInfo;
		Q_ASSERT(!r || this->parent == another.parent);
		Q_ASSERT(!r || this->mapped == another.mapped);
		Q_ASSERT(!r || this->children == another.children);
		return r;
	}

	QFileInfo fileInfo;
	QVector<NodeInfo> children;
	NodeInfo* parent;

	bool mapped;
};

FilesystemModel::~FilesystemModel()
{}

QModelIndex FilesystemModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!hasIndex(row, column, parent)) {
		return QModelIndex();
	}

	if (!parent.isValid()) {
		Q_ASSERT(_nodes.size() > row);
		return createIndex(row, column, const_cast<NodeInfo*>(&_nodes[row]));
	}

	NodeInfo* parentInfo = static_cast<NodeInfo*>(parent.internalPointer());
	Q_ASSERT(parentInfo != 0);
	Q_ASSERT(parentInfo->mapped);
	Q_ASSERT(parentInfo->children.size() > row);
	return createIndex(row, column, &parentInfo->children[row]);
}

QModelIndex FilesystemModel::parent(const QModelIndex &child) const
{
	if (!child.isValid()) {
		return QModelIndex();
	}

	NodeInfo* childInfo = static_cast<NodeInfo*>(child.internalPointer());
	Q_ASSERT(childInfo != 0);
	NodeInfo* parentInfo = childInfo->parent;
	if (parentInfo != 0) {
		return createIndex(findRow(parentInfo), RamificationColumn, parentInfo);
	}
	else {
		return QModelIndex();
	}
}

int FilesystemModel::findRow(const NodeInfo *nodeInfo) const
{
	Q_ASSERT(nodeInfo != 0);
	const NodeInfoList& parentInfoChildren = nodeInfo->parent != 0 ? nodeInfo->parent->children: _nodes;
	NodeInfoList::const_iterator position = qFind(parentInfoChildren, *nodeInfo);
	Q_ASSERT(position != parentInfoChildren.end());
	return std::distance(parentInfoChildren.begin(), position);
}

int FilesystemModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid()) {
		return _nodes.size();
	}
	const NodeInfo* parentInfo = static_cast<const NodeInfo*>(parent.internalPointer());
	Q_ASSERT(parentInfo != 0);

	return parentInfo->children.size();
}

bool FilesystemModel::hasChildren(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		const NodeInfo* parentInfo = static_cast<const NodeInfo*>(parent.internalPointer());
		Q_ASSERT(parentInfo != 0);
		if (!parentInfo->mapped) {
			return true;//QDir(parentInfo->fileInfo.absoluteFilePath()).count() > 0;
		}
	}
	return QAbstractItemModel::hasChildren(parent);
}

int FilesystemModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return ColumnCount;
}

QVariant FilesystemModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	const NodeInfo* nodeInfo = static_cast<NodeInfo*>(index.internalPointer());
	const QFileInfo& fileInfo = nodeInfo->fileInfo;
	Q_ASSERT(nodeInfo != 0);

	switch (index.column()) {
	case NameColumn:
		return nameData(fileInfo, role);
	case ModificationDateColumn:
		if (role == Qt::DisplayRole) {
			return fileInfo.lastModified();
		}
		break;
	case SizeColumn:
		if (role == Qt::DisplayRole) {
			return fileInfo.isDir()? QVariant(): fileInfo.size();
		}
		break;
	case TypeColumn:
		if (role == Qt::DisplayRole) {
			return _metaProvider->type(fileInfo);
		}
		break;
	default:
		break;
	}
	return QVariant();
}

QVariant FilesystemModel::nameData(const QFileInfo &fileInfo, int role) const
{
	switch (role) {
	case Qt::EditRole:
		return fileInfo.fileName();
	case Qt::DisplayRole:
		if (fileInfo.isRoot()) {
			return fileInfo.absoluteFilePath();
		}
		else if (fileInfo.isDir()){
			return fileInfo.fileName();
		}
		else {
			return fileInfo.completeBaseName();
		}
	case Qt::DecorationRole:
		return _metaProvider->icon(fileInfo);
	default:
		return QVariant();
	}
	Q_UNREACHABLE();
}

bool FilesystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid()) {
		return false;
	}
	if (role != Qt::EditRole) {
		return false;
	}
	if (index.column() != NameColumn) {
		return false;
	}

	QString newName = value.toString();
	if (newName.contains('/') || newName.contains(QDir::separator())) {
		return false;
	}
	NodeInfo* nodeInfo = static_cast<NodeInfo*>(index.internalPointer());
	QString fullNewName = nodeInfo->fileInfo.absoluteDir().path() +"/" + newName;
	QString fullOldName = nodeInfo->fileInfo.absoluteFilePath();
	qDebug() << fullOldName << fullNewName;
	bool renamed = QFile::rename(fullOldName, fullNewName);
	qDebug() << renamed;
	if (renamed) {
		nodeInfo->fileInfo = QFileInfo(fullNewName);
		emit dataChanged(index, index.sibling(index.row(), ColumnCount));
	}
	return renamed;
}

QVariant FilesystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	const QStringList headers = {"Имя", "Дата изменения", "Размер", "Тип"};
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section < headers.size()) {
		return headers[section];
	}
	return QVariant();
}

bool FilesystemModel::canFetchMore(const QModelIndex &parent) const
{
	if (!parent.isValid()) {
		return false;
	}

	const NodeInfo* parentInfo = static_cast<const NodeInfo*>(parent.internalPointer());
	Q_ASSERT(parentInfo != 0);
	return !parentInfo->mapped;
}

void FilesystemModel::fetchMore(const QModelIndex &parent)
{
	Q_ASSERT(parent.isValid());
	NodeInfo* parentInfo = static_cast<NodeInfo*>(parent.internalPointer());
	Q_ASSERT(parentInfo != 0);
	Q_ASSERT(!parentInfo->mapped);

	const QFileInfo& fileInfo = parentInfo->fileInfo;
	Q_ASSERT(fileInfo.isDir());

	QDir dir = QDir(fileInfo.absoluteFilePath());
	QFileInfoList children = dir.entryInfoList(QStringList(), QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);

	int insrtCnt = children.size() - 1;
	if (insrtCnt < 0) {
		insrtCnt = 0;
	}
	beginInsertRows(parent, 0, insrtCnt);
	parentInfo->children.reserve(children.size());
	for (const QFileInfo& entry: children) {
		NodeInfo nodeInfo(entry, parentInfo);
		nodeInfo.mapped = !entry.isDir();
		parentInfo->children.push_back(std::move(nodeInfo));
	}
	parentInfo->mapped = true;
	endInsertRows();
}

void FilesystemModel::fetchRootDirectory()
{
	const QFileInfoList drives = QDir::drives();
	qCopy(drives.begin(), drives.end(), std::back_inserter(_nodes));
}

Qt::ItemFlags FilesystemModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	if (index.isValid() && index.column() == NameColumn) {
		const NodeInfo* nodeInfo = static_cast<const NodeInfo*>(index.internalPointer());
		if (!nodeInfo->fileInfo.isRoot()) {
			flags |= Qt::ItemIsEditable;
		}
	}
	return flags;
}
