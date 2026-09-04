#include "DiskTreeModel.h"
#include <QFileInfo>
#include <QColor>

DiskTreeModel::DiskTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

void DiskTreeModel::setRootNode(DiskNode* rootNode) {
    beginResetModel();
    m_rootNode = rootNode;
    endResetModel();
}

DiskNode* DiskTreeModel::nodeForIndex(const QModelIndex& index) const {
    if (!index.isValid()) return m_rootNode;
    return static_cast<DiskNode*>(index.internalPointer());
}

QModelIndex DiskTreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    DiskNode* parentNode = nodeForIndex(parent);
    if (!parentNode) return QModelIndex();

    if (row >= 0 && static_cast<size_t>(row) < parentNode->children().size()) {
        DiskNode* child = parentNode->children()[row].get();
        return createIndex(row, column, child);
    }
    return QModelIndex();
}

QModelIndex DiskTreeModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return QModelIndex();

    DiskNode* childNode = static_cast<DiskNode*>(child.internalPointer());
    if (!childNode || childNode == m_rootNode) return QModelIndex();

    DiskNode* parentNode = childNode->parent();
    if (!parentNode || parentNode == m_rootNode) return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

int DiskTreeModel::rowCount(const QModelIndex& parent) const {
    if (parent.column() > 0) return 0;

    DiskNode* parentNode = nodeForIndex(parent);
    if (!parentNode) return 0;
    return static_cast<int>(parentNode->children().size());
}

int DiskTreeModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return ColCount;
}

QVariant DiskTreeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    DiskNode* node = static_cast<DiskNode*>(index.internalPointer());
    if (!node) return QVariant();

    int col = index.column();

    if (role == Qt::DisplayRole) {
        switch (col) {
            case ColName:
                return node->name();
            case ColSize:
                return DiskNode::formatSize(node->size());
            case ColFiles:
                return node->isDirectory() ? QString::number(node->fileCount()) : QStringLiteral("-");
            case ColSubfolders:
                return node->isDirectory() ? QString::number(node->dirCount()) : QStringLiteral("-");
            case ColType:
                return node->isDirectory() ? QStringLiteral("Folder") : node->extension().toUpper();
            default:
                return QVariant();
        }
    }

    if (role == Qt::DecorationRole && col == ColName) {
        if (node->isDirectory()) {
            return m_iconProvider.icon(QFileIconProvider::Folder);
        } else {
            return m_iconProvider.icon(QFileIconProvider::File);
        }
    }

    if (role == Qt::UserRole) {
        // UserRole is used by delegates and sorting
        switch (col) {
            case ColUsage: {
                double parentSize = 0;
                if (node->parent() && node->parent()->size() > 0) {
                    parentSize = static_cast<double>(node->parent()->size());
                } else if (m_rootNode && m_rootNode->size() > 0) {
                    parentSize = static_cast<double>(m_rootNode->size());
                }
                if (parentSize > 0) {
                    return (static_cast<double>(node->size()) / parentSize) * 100.0;
                }
                return 0.0;
            }
            case ColSize:
                return static_cast<qulonglong>(node->size());
            case ColFiles:
                return static_cast<qulonglong>(node->fileCount());
            case ColSubfolders:
                return static_cast<qulonglong>(node->dirCount());
            default:
                return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (col == ColSize || col == ColFiles || col == ColSubfolders) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        if (col == ColUsage) {
            return QVariant(Qt::AlignCenter);
        }
    }

    if (role == Qt::ForegroundRole && col == ColType && !node->isDirectory()) {
        return DiskNode::getColorForExtension(node->extension());
    }

    return QVariant();
}

QVariant DiskTreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal) return QVariant();

    if (role == Qt::DisplayRole) {
        switch (section) {
            case ColName:       return QStringLiteral("Name");
            case ColUsage:      return QStringLiteral("Usage %");
            case ColSize:       return QStringLiteral("Size");
            case ColFiles:      return QStringLiteral("Files");
            case ColSubfolders: return QStringLiteral("Folders");
            case ColType:       return QStringLiteral("Type");
            default:            return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (section == ColSize || section == ColFiles || section == ColSubfolders) {
            return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
        if (section == ColUsage) {
            return QVariant(Qt::AlignCenter);
        }
    }

    return QVariant();
}

QModelIndex DiskTreeModel::indexForNode(DiskNode* node) const {
    if (!node || node == m_rootNode) return QModelIndex();

    DiskNode* parentNode = node->parent();
    if (!parentNode) return QModelIndex();

    QModelIndex parentIndex = (parentNode == m_rootNode) ? QModelIndex() : indexForNode(parentNode);
    return index(node->row(), 0, parentIndex);
}
