#pragma once

#include <QAbstractItemModel>
#include <QFileIconProvider>
#include "DiskNode.h"

class DiskTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum Column {
        ColName = 0,
        ColUsage,
        ColSize,
        ColFiles,
        ColSubfolders,
        ColType,
        ColCount
    };

    explicit DiskTreeModel(QObject* parent = nullptr);
    ~DiskTreeModel() override = default;

    void setRootNode(DiskNode* rootNode);
    DiskNode* rootNode() const { return m_rootNode; }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    DiskNode* nodeForIndex(const QModelIndex& index) const;
    QModelIndex indexForNode(DiskNode* node) const;

private:
    DiskNode* m_rootNode{nullptr};
    QFileIconProvider m_iconProvider;
};
