#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QLineEdit>
#include <QComboBox>
#include <QMenu>
#include <QAction>
#include "core/DuplicateFinder.h"

class DiskNode;

/**
 * @brief DuplicateFilesWidget provides an interactive visualization and management
 * interface for finding and safely purging duplicate files across the scanned filesystem.
 */
class DuplicateFilesWidget : public QWidget {
    Q_OBJECT

public:
    explicit DuplicateFilesWidget(QWidget* parent = nullptr);
    ~DuplicateFilesWidget() override;

    /**
     * @brief Sets the root DiskNode from which duplicates will be computed.
     */
    void setRootNode(DiskNode* rootNode);

    /**
     * @brief Resets all UI elements, KPIs, and results tree.
     */
    void clear();

public slots:
    void startScan();
    void cancelScan();
    void selectKeepNewest();
    void selectKeepOldest();
    void selectAllDuplicates();
    void deselectAll();
    void deleteSelectedToTrash();
    void filterFiles(const QString& query);

private slots:
    void onScanStarted();
    void onScanFinished(const DuplicateScanResult& result);
    void onScanCancelled();
    void onItemChanged(QTreeWidgetItem* item, int column);
    void showContextMenu(const QPoint& pos);

private:
    void setupUi();
    void createKpiHeader();
    void createActionToolbar();
    void createResultsTree();
    void populateTree();
    void updateKpiDisplay();
    void updateSelectedStats();

    DiskNode* m_rootNode{nullptr};
    DuplicateFinder* m_finder{nullptr};
    DuplicateScanResult m_result;

    // KPI Metric Labels
    QLabel* m_wastedSpaceLabel{nullptr};
    QLabel* m_duplicateGroupsLabel{nullptr};
    QLabel* m_duplicateFilesLabel{nullptr};
    QLabel* m_selectedCountLabel{nullptr};

    // Action Controls
    QPushButton* m_scanBtn{nullptr};
    QPushButton* m_cancelBtn{nullptr};
    QComboBox* m_algoCombo{nullptr};
    QLineEdit* m_searchEdit{nullptr};
    QPushButton* m_keepNewestBtn{nullptr};
    QPushButton* m_keepOldestBtn{nullptr};
    QPushButton* m_selectAllBtn{nullptr};
    QPushButton* m_deselectAllBtn{nullptr};
    QPushButton* m_deleteBtn{nullptr};

    // Progress & Status
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_statusLabel{nullptr};

    // Tree View
    QTreeWidget* m_treeWidget{nullptr};

    // State tracking
    bool m_isUpdatingCheckState{false};
};
