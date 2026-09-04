#pragma once

#include <QWidget>
#include <QTableWidget>
#include <map>
#include "DiskNode.h"

struct ExtensionSummary {
    QString ext;
    QString category;
    QColor color;
    uint64_t totalBytes{0};
    uint64_t fileCount{0};
};

class ExtensionStatsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ExtensionStatsWidget(QWidget* parent = nullptr);

    void populateFromNode(DiskNode* rootNode);
    void clear();

private:
    void collectStats(DiskNode* node, std::map<QString, ExtensionSummary>& stats);

    QTableWidget* m_table{nullptr};
};
