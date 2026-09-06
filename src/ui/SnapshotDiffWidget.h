#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <memory>
#include "SnapshotEngine.h"

enum class DiffFilter {
    All,
    GrowthOnly,
    FreedOnly,
    AddedOnly,
    DeletedOnly
};

class SnapshotDiffWidget : public QWidget {
    Q_OBJECT

public:
    explicit SnapshotDiffWidget(QWidget* parent = nullptr);
    ~SnapshotDiffWidget() override = default;

    void setDiffData(std::unique_ptr<DiffNode> diffRoot, const DiffSummary& summary);
    void clear();

signals:
    void fileSelected(const QString& path);

private slots:
    void onFilterChanged(DiffFilter filter);
    void onSearchTextChanged(const QString& text);
    void onTreeContextMenu(const QPoint& pos);

private:
    void setupUi();
    void updateSummaryCards(const DiffSummary& summary);
    void populateTree();
    void addDiffItem(QTreeWidgetItem* parentItem, const DiffNode* node, const QString& searchLower);
    bool shouldIncludeNode(const DiffNode* node, const QString& searchLower) const;

    // Header KPI widgets
    QLabel* m_lblOldScan{nullptr};
    QLabel* m_lblNewScan{nullptr};
    QLabel* m_lblNetChange{nullptr};
    QLabel* m_lblCounts{nullptr};

    // Filter controls
    QPushButton* m_btnFilterAll{nullptr};
    QPushButton* m_btnFilterGrowth{nullptr};
    QPushButton* m_btnFilterFreed{nullptr};
    QPushButton* m_btnFilterAdded{nullptr};
    QPushButton* m_btnFilterDeleted{nullptr};
    QLineEdit* m_searchEdit{nullptr};

    // Tree View
    QTreeWidget* m_treeWidget{nullptr};

    std::unique_ptr<DiffNode> m_diffRoot;
    DiffSummary m_summary;
    DiffFilter m_currentFilter{DiffFilter::All};
};
