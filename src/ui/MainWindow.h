#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QTreeView>
#include <QTabWidget>
#include <QProgressBar>
#include <QLabel>
#include <QSplitter>
#include <memory>

#include "DiskNode.h"
#include "ScannerEngine.h"
#include "DiskTreeModel.h"
#include "SizeBarDelegate.h"
#include "TreemapWidget.h"
#include "SunburstWidget.h"
#include "BreadcrumbWidget.h"
#include "ExtensionStatsWidget.h"
#include "TopFilesWidget.h"
#include "SnapshotDiffWidget.h"

class QStackedWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onSelectFolderClicked();
    void onDriveSelected(int index);
    void onStartScanClicked();
    void onCancelScanClicked();

    void onScanStarted(const QString& rootPath);
    void onScanProgress(quint64 files, quint64 bytes, const QString& currentFolder);
    void onScanFinished(std::shared_ptr<DiskNode> rootNode, qint64 elapsedMs, bool wasCancelled);
    void onScanError(const QString& message);

    void onTreeSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void onTreemapNodeSelected(DiskNode* node);
    void onTreemapNodeDoubleClicked(DiskNode* node);
    void onTreemapRootChanged(DiskNode* newRoot);
    void onSunburstNodeSelected(DiskNode* node);
    void onSunburstNodeDoubleClicked(DiskNode* node);
    void onSunburstRootChanged(DiskNode* newRoot);
    void onBreadcrumbFolderSelected(DiskNode* node);
    void onBreadcrumbUpRequested();
    void onBreadcrumbResetRequested();
    void onTopFileSelected(DiskNode* node);
    void setVisualizerView(int index);

    // Export & Snapshot slots
    void onExportHtmlClicked();
    void onExportCsvClicked();
    void onExportJsonClicked();
    void onSaveSnapshotClicked();
    void onCompareWithSnapshotClicked();
    void onCompareTwoSnapshotsClicked();

private:
    void setupUi();
    void applyDarkTheme();
    void populateDrives();

    // Top Controls
    QComboBox* m_driveCombo{nullptr};
    QPushButton* m_btnBrowse{nullptr};
    QPushButton* m_btnScan{nullptr};
    QPushButton* m_btnCancel{nullptr};
    QPushButton* m_btnExport{nullptr};
    QPushButton* m_btnSnapshot{nullptr};
    BreadcrumbWidget* m_breadcrumb{nullptr};

    // Central Splitter & Views
    QSplitter* m_mainSplitter{nullptr};
    QTabWidget* m_tabs{nullptr};
    QTreeView* m_treeView{nullptr};
    DiskTreeModel* m_treeModel{nullptr};
    ExtensionStatsWidget* m_extStatsWidget{nullptr};
    TopFilesWidget* m_topFilesWidget{nullptr};
    SnapshotDiffWidget* m_snapshotDiffWidget{nullptr};

    // Visualization Stack & Switcher
    QWidget* m_visContainer{nullptr};
    QStackedWidget* m_visStack{nullptr};
    TreemapWidget* m_treemapWidget{nullptr};
    SunburstWidget* m_sunburstWidget{nullptr};
    QPushButton* m_btnTreemapView{nullptr};
    QPushButton* m_btnSunburstView{nullptr};

    // Status Bar
    QLabel* m_statusLabel{nullptr};
    QLabel* m_statsLabel{nullptr};
    QLabel* m_timeLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};

    // Scanner & Data
    ScannerEngine* m_scanner{nullptr};
    std::shared_ptr<DiskNode> m_rootNode;
    QString m_currentScanPath;
};
