#pragma once

#include <QWidget>
#include <QTableWidget>
#include <vector>
#include "DiskNode.h"

class TopFilesWidget : public QWidget {
    Q_OBJECT

public:
    explicit TopFilesWidget(QWidget* parent = nullptr);

    void populateFromNode(DiskNode* rootNode, int maxCount = 100);
    void clear();

signals:
    void fileSelected(DiskNode* fileNode);

private:
    void collectFiles(DiskNode* node, std::vector<DiskNode*>& files);

    QTableWidget* m_table{nullptr};
    std::vector<DiskNode*> m_topNodes;
};
