#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QDir>
#include <QStorageInfo>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QStackedWidget>
#include <QDesktopServices>
#include <QShortcut>
#include "ReportExporter.h"
#include "SnapshotEngine.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_scanner(new ScannerEngine(this))
{
    setupUi();
    applyDarkTheme();
    populateDrives();

    // Scanner signals
    connect(m_scanner, &ScannerEngine::scanStarted, this, &MainWindow::onScanStarted);
    connect(m_scanner, &ScannerEngine::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_scanner, &ScannerEngine::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_scanner, &ScannerEngine::scanError, this, &MainWindow::onScanError);

    // TreeView signals
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onTreeSelectionChanged);

    // TreeView Context Menu
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested, this, [this](const QPoint& pos) {
        QModelIndex idx = m_treeView->indexAt(pos);
        if (!idx.isValid()) return;

        DiskNode* node = m_treeModel->nodeForIndex(idx);
        if (!node) return;

        QMenu menu(this);
        QAction* actOpen = menu.addAction(QStringLiteral("Open in File Explorer"));
        QAction* actCopy = menu.addAction(QStringLiteral("Copy Full Path"));
        QAction* actZoom = nullptr;
        if (node->isDirectory()) {
            menu.addSeparator();
            actZoom = menu.addAction(QStringLiteral("Zoom Visualizer to this folder"));
        }

        QAction* selected = menu.exec(m_treeView->viewport()->mapToGlobal(pos));
        if (selected == actOpen) {
#ifdef _WIN32
            QString path = QDir::toNativeSeparators(node->fullPath());
            QString param = QStringLiteral("/select,\"%1\"").arg(path);
            QProcess::startDetached(QStringLiteral("explorer.exe"), {param});
#endif
        } else if (selected == actCopy) {
            QGuiApplication::clipboard()->setText(node->fullPath());
        } else if (actZoom && selected == actZoom) {
            m_treemapWidget->zoomIn(node);
            m_sunburstWidget->zoomIn(node);
        }
    });

    // Treemap signals
    connect(m_treemapWidget, &TreemapWidget::nodeSelected, this, &MainWindow::onTreemapNodeSelected);
    connect(m_treemapWidget, &TreemapWidget::nodeDoubleClicked, this, &MainWindow::onTreemapNodeDoubleClicked);
    connect(m_treemapWidget, &TreemapWidget::currentRootChanged, this, &MainWindow::onTreemapRootChanged);

    // Sunburst signals
    connect(m_sunburstWidget, &SunburstWidget::nodeSelected, this, &MainWindow::onSunburstNodeSelected);
    connect(m_sunburstWidget, &SunburstWidget::nodeDoubleClicked, this, &MainWindow::onSunburstNodeDoubleClicked);
    connect(m_sunburstWidget, &SunburstWidget::currentRootChanged, this, &MainWindow::onSunburstRootChanged);

    // Breadcrumb signals
    connect(m_breadcrumb, &BreadcrumbWidget::folderSelected, this, &MainWindow::onBreadcrumbFolderSelected);
    connect(m_breadcrumb, &BreadcrumbWidget::upRequested, this, &MainWindow::onBreadcrumbUpRequested);
    connect(m_breadcrumb, &BreadcrumbWidget::resetRequested, this, &MainWindow::onBreadcrumbResetRequested);

    // Top files signal
    connect(m_topFilesWidget, &TopFilesWidget::fileSelected, this, &MainWindow::onTopFileSelected);

    // Global Keyboard Shortcut: F5 to re-scan / refresh
    QShortcut* shortcutF5 = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(shortcutF5, &QShortcut::activated, this, [this]() {
        if (m_btnScan && m_btnScan->isEnabled()) {
            onStartScanClicked();
        }
    });

    // Global Keyboard Shortcut: Backspace and Alt+Up to navigate up one folder level
    QShortcut* shortcutBack = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(shortcutBack, &QShortcut::activated, this, &MainWindow::onBreadcrumbUpRequested);

    QShortcut* shortcutAltUp = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Up), this);
    connect(shortcutAltUp, &QShortcut::activated, this, &MainWindow::onBreadcrumbUpRequested);

    // Global Keyboard Shortcut: Ctrl+O to browse folder
    QShortcut* shortcutCtrlO = new QShortcut(QKeySequence::Open, this);
    connect(shortcutCtrlO, &QShortcut::activated, this, [this]() {
        if (m_btnBrowse && m_btnBrowse->isEnabled()) {
            onSelectFolderClicked();
        }
    });

    resize(1200, 800);
}

void MainWindow::setupUi() {
    setWindowTitle(QStringLiteral("Mem-Map - High Performance Storage Visualizer (C++ & Qt6)"));

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(12, 12, 12, 6);
    rootLayout->setSpacing(8);

    // Top Bar 1: Drive selection and actions
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->setSpacing(8);

    QLabel* lblDrive = new QLabel(QStringLiteral("Target:"), this);
    lblDrive->setStyleSheet(QStringLiteral("font-weight: bold; color: #E6EDF3;"));

    m_driveCombo = new QComboBox(this);
    m_driveCombo->setMinimumWidth(220);
    connect(m_driveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDriveSelected);

    m_btnBrowse = new QPushButton(QStringLiteral("📁 Browse Folder..."), this);
    connect(m_btnBrowse, &QPushButton::clicked, this, &MainWindow::onSelectFolderClicked);

    m_btnScan = new QPushButton(QStringLiteral("⚡ Scan Now"), this);
    m_btnScan->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #238636; color: #FFFFFF; font-weight: bold; border-radius: 6px; padding: 6px 16px; border: 1px solid #2EA043; }"
        "QPushButton:hover { background-color: #2EA043; }"
        "QPushButton:pressed { background-color: #1F7F32; }"
    ));
    connect(m_btnScan, &QPushButton::clicked, this, &MainWindow::onStartScanClicked);

    m_btnCancel = new QPushButton(QStringLiteral("✖ Cancel"), this);
    m_btnCancel->setEnabled(false);
    m_btnCancel->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #DA3633; color: #FFFFFF; font-weight: bold; border-radius: 6px; padding: 6px 14px; border: 1px solid #F85149; }"
        "QPushButton:hover { background-color: #F85149; }"
        "QPushButton:disabled { background-color: #373E47; color: #768390; border-color: #444C56; }"
    ));
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelScanClicked);

    topBar->addWidget(lblDrive);
    topBar->addWidget(m_driveCombo);
    topBar->addWidget(m_btnBrowse);
    topBar->addWidget(m_btnScan);
    topBar->addWidget(m_btnCancel);

    // Export button with dropdown menu
    m_btnExport = new QPushButton(QStringLiteral("📑 Export Report ▾"), this);
    QMenu* exportMenu = new QMenu(m_btnExport);
    QAction* actHtml = exportMenu->addAction(QStringLiteral("🌐 Interactive HTML Report (.html)..."));
    QAction* actCsv  = exportMenu->addAction(QStringLiteral("📊 Spreadsheet Data (.csv)..."));
    QAction* actJson = exportMenu->addAction(QStringLiteral("📄 Raw Tree Data (.json)..."));
    m_btnExport->setMenu(exportMenu);
    connect(actHtml, &QAction::triggered, this, &MainWindow::onExportHtmlClicked);
    connect(actCsv, &QAction::triggered, this, &MainWindow::onExportCsvClicked);
    connect(actJson, &QAction::triggered, this, &MainWindow::onExportJsonClicked);
    topBar->addWidget(m_btnExport);

    // Snapshot button with dropdown menu
    m_btnSnapshot = new QPushButton(QStringLiteral("💾 Snapshot ▾"), this);
    QMenu* snapshotMenu = new QMenu(m_btnSnapshot);
    QAction* actSaveSnap = snapshotMenu->addAction(QStringLiteral("💾 Save Current Scan as Snapshot (.mmap)..."));
    snapshotMenu->addSeparator();
    QAction* actCompareWith = snapshotMenu->addAction(QStringLiteral("⚖ Compare Current Scan with Snapshot..."));
    QAction* actCompareTwo  = snapshotMenu->addAction(QStringLiteral("🔄 Compare Two Saved Snapshots..."));
    m_btnSnapshot->setMenu(snapshotMenu);
    connect(actSaveSnap, &QAction::triggered, this, &MainWindow::onSaveSnapshotClicked);
    connect(actCompareWith, &QAction::triggered, this, &MainWindow::onCompareWithSnapshotClicked);
    connect(actCompareTwo, &QAction::triggered, this, &MainWindow::onCompareTwoSnapshotsClicked);
    topBar->addWidget(m_btnSnapshot);

    topBar->addStretch();

    rootLayout->addLayout(topBar);

    // Top Bar 2: Breadcrumbs
    m_breadcrumb = new BreadcrumbWidget(this);
    rootLayout->addWidget(m_breadcrumb);

    // Central Splitter (Left: Tabs with TreeView, Right: Treemap)
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(6);

    // Tabs container
    m_tabs = new QTabWidget(this);

    // Tab 1: Tree View
    m_treeView = new QTreeView(this);
    m_treeModel = new DiskTreeModel(this);
    m_treeView->setModel(m_treeModel);
    m_treeView->setItemDelegateForColumn(DiskTreeModel::ColUsage, new SizeBarDelegate(this));
    m_treeView->setSortingEnabled(false);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setAnimated(true);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(DiskTreeModel::ColName, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(DiskTreeModel::ColUsage, QHeaderView::Interactive);
    m_treeView->header()->setSectionResizeMode(DiskTreeModel::ColSize, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(DiskTreeModel::ColFiles, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(DiskTreeModel::ColSubfolders, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(DiskTreeModel::ColType, QHeaderView::ResizeToContents);
    m_treeView->header()->resizeSection(DiskTreeModel::ColUsage, 120);

    m_tabs->addTab(m_treeView, QStringLiteral("Directory Tree"));

    // Tab 2: File Types breakdown
    m_extStatsWidget = new ExtensionStatsWidget(this);
    m_tabs->addTab(m_extStatsWidget, QStringLiteral("File Types"));

    // Tab 3: Top Largest Files
    m_topFilesWidget = new TopFilesWidget(this);
    m_tabs->addTab(m_topFilesWidget, QStringLiteral("Top 100 Files"));

    // Tab 4: Snapshot Diff View
    m_snapshotDiffWidget = new SnapshotDiffWidget(this);
    m_tabs->addTab(m_snapshotDiffWidget, QStringLiteral("Snapshot Diff"));

    // Right Pane: Visualization Container with View Switcher
    m_visContainer = new QWidget(this);
    QVBoxLayout* visLayout = new QVBoxLayout(m_visContainer);
    visLayout->setContentsMargins(0, 0, 0, 0);
    visLayout->setSpacing(6);

    // View Switcher Bar
    QHBoxLayout* visHeader = new QHBoxLayout();
    visHeader->setContentsMargins(4, 0, 4, 0);
    visHeader->setSpacing(6);

    QLabel* lblVis = new QLabel(QStringLiteral("Visualizer:"), this);
    lblVis->setStyleSheet(QStringLiteral("color: #8B949E; font-weight: bold; font-size: 12px;"));
    visHeader->addWidget(lblVis);

    m_btnTreemapView = new QPushButton(QStringLiteral("▦ Treemap"), this);
    m_btnSunburstView = new QPushButton(QStringLiteral("🔘 Sunburst (DaisyDisk)"), this);

    m_btnTreemapView->setCheckable(true);
    m_btnSunburstView->setCheckable(true);
    m_btnTreemapView->setChecked(true);
    m_btnTreemapView->setObjectName(QStringLiteral("visToggle"));
    m_btnSunburstView->setObjectName(QStringLiteral("visToggle"));

    connect(m_btnTreemapView, &QPushButton::clicked, this, [this]() { setVisualizerView(0); });
    connect(m_btnSunburstView, &QPushButton::clicked, this, [this]() { setVisualizerView(1); });

    visHeader->addWidget(m_btnTreemapView);
    visHeader->addWidget(m_btnSunburstView);
    visHeader->addStretch();
    visLayout->addLayout(visHeader);

    // Stacked Visualizers
    m_visStack = new QStackedWidget(this);
    m_treemapWidget = new TreemapWidget(this);
    m_sunburstWidget = new SunburstWidget(this);
    m_visStack->addWidget(m_treemapWidget);
    m_visStack->addWidget(m_sunburstWidget);
    visLayout->addWidget(m_visStack, 1);

    m_mainSplitter->addWidget(m_tabs);
    m_mainSplitter->addWidget(m_visContainer);
    m_mainSplitter->setStretchFactor(0, 5);
    m_mainSplitter->setStretchFactor(1, 5);

    rootLayout->addWidget(m_mainSplitter, 1);

    // Bottom Status Bar
    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins(4, 2, 4, 2);
    statusLayout->setSpacing(12);

    m_statusLabel = new QLabel(QStringLiteral("Ready. Select a drive or folder to scan."), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8B949E;"));

    m_statsLabel = new QLabel(QStringLiteral("Files: 0 | Total: 0 B"), this);
    m_statsLabel->setStyleSheet(QStringLiteral("color: #58A6FF; font-weight: bold;"));

    m_timeLabel = new QLabel(QStringLiteral("Time: 00:00.0"), this);
    m_timeLabel->setStyleSheet(QStringLiteral("color: #7EE787;"));

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumHeight(14);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedWidth(140);

    statusLayout->addWidget(m_statusLabel, 1);
    statusLayout->addWidget(m_statsLabel);
    statusLayout->addWidget(m_timeLabel);
    statusLayout->addWidget(m_progressBar);

    rootLayout->addLayout(statusLayout);
}

void MainWindow::applyDarkTheme() {
    QString qss = QStringLiteral(
        "QMainWindow { background-color: #0D1117; }"
        "QWidget { color: #C9D1D9; font-family: 'Segoe UI', 'SF Pro Display', sans-serif; font-size: 13px; }"
        "QComboBox, QPushButton, QLineEdit { background-color: #21262D; border: 1px solid #30363D; border-radius: 6px; padding: 5px 10px; color: #C9D1D9; }"
        "QComboBox:hover, QPushButton:hover { background-color: #30363D; border-color: #8B949E; }"
        "QPushButton#visToggle { background-color: #21262D; border: 1px solid #30363D; border-radius: 4px; padding: 4px 12px; color: #8B949E; font-weight: bold; font-size: 12px; }"
        "QPushButton#visToggle:checked { background-color: #1F6FEB; border-color: #388BFD; color: #FFFFFF; }"
        "QPushButton#visToggle:hover:!checked { background-color: #30363D; color: #C9D1D9; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox QAbstractItemView { background-color: #161B22; border: 1px solid #30363D; selection-background-color: #1F6FEB; color: #C9D1D9; }"
        "QTabWidget::pane { border: 1px solid #30363D; background-color: #161B22; border-radius: 6px; }"
        "QTabBar::tab { background-color: #0D1117; color: #8B949E; padding: 8px 16px; border-top-left-radius: 6px; border-top-right-radius: 6px; border: 1px solid #30363D; border-bottom: none; margin-right: 2px; }"
        "QTabBar::tab:selected { background-color: #161B22; color: #58A6FF; font-weight: bold; border-color: #30363D; }"
        "QTabBar::tab:hover:!selected { background-color: #1F242C; color: #C9D1D9; }"
        "QTreeView, QTableWidget { background-color: #161B22; border: none; gridline-color: #21262D; alternate-background-color: #0D1117; }"
        "QTreeView::item, QTableWidget::item { padding: 4px; border: none; }"
        "QTreeView::item:hover, QTableWidget::item:hover { background-color: #1F242C; }"
        "QTreeView::item:selected, QTableWidget::item:selected { background-color: #1F6FEB; color: #FFFFFF; }"
        "QHeaderView::section { background-color: #0D1117; color: #8B949E; padding: 5px 8px; border: none; border-bottom: 1px solid #30363D; border-right: 1px solid #21262D; font-weight: bold; font-size: 12px; }"
        "QProgressBar { background-color: #21262D; border: 1px solid #30363D; border-radius: 4px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1F6FEB, stop:1 #58A6FF); border-radius: 3px; }"
        "QSplitter::handle { background-color: #21262D; }"
        "QSplitter::handle:hover { background-color: #58A6FF; }"
        "QScrollBar:vertical { background-color: #0D1117; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background-color: #30363D; min-height: 20px; border-radius: 4px; }"
        "QScrollBar::handle:vertical:hover { background-color: #58A6FF; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background-color: #0D1117; height: 10px; margin: 0; }"
        "QScrollBar::handle:horizontal { background-color: #30363D; min-width: 20px; border-radius: 4px; }"
        "QScrollBar::handle:horizontal:hover { background-color: #58A6FF; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QMenu { background-color: #161B22; border: 1px solid #30363D; color: #C9D1D9; padding: 4px; }"
        "QMenu::item { padding: 6px 24px 6px 12px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #1F6FEB; color: #FFFFFF; }"
        "QToolTip { background-color: #161B22; border: 1px solid #58A6FF; color: #FFFFFF; padding: 6px; border-radius: 4px; }"
    );
    setStyleSheet(qss);
}

void MainWindow::populateDrives() {
    m_driveCombo->clear();
    const auto drives = QDir::drives();

    for (const QFileInfo& drive : drives) {
        QString path = drive.absoluteFilePath();
        QStorageInfo storage(path);
        if (storage.isValid() && storage.isReady()) {
            qint64 total = storage.bytesTotal();
            qint64 free = storage.bytesAvailable();
            QString label = QStringLiteral("%1 (%2 free of %3)")
                .arg(path)
                .arg(DiskNode::formatSize(free))
                .arg(DiskNode::formatSize(total));
            m_driveCombo->addItem(label, path);
        } else {
            m_driveCombo->addItem(path, path);
        }
    }

    if (m_driveCombo->count() > 0) {
        m_currentScanPath = m_driveCombo->currentData().toString();
    }
}

void MainWindow::onDriveSelected(int index) {
    if (index >= 0) {
        m_currentScanPath = m_driveCombo->itemData(index).toString();
    }
}

void MainWindow::onSelectFolderClicked() {
    QString folder = QFileDialog::getExistingDirectory(this, QStringLiteral("Select Folder to Analyze"), m_currentScanPath);
    if (!folder.isEmpty()) {
        m_currentScanPath = folder;
        // Check if in combo, otherwise prepend
        int idx = m_driveCombo->findData(folder);
        if (idx >= 0) {
            m_driveCombo->setCurrentIndex(idx);
        } else {
            m_driveCombo->insertItem(0, QStringLiteral("📁 ") + folder, folder);
            m_driveCombo->setCurrentIndex(0);
        }
    }
}

void MainWindow::onStartScanClicked() {
    if (m_currentScanPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Selection Required"), QStringLiteral("Please select a valid drive or folder to scan."));
        return;
    }

    m_scanner->startScan(m_currentScanPath);
}

void MainWindow::onCancelScanClicked() {
    m_scanner->cancelScan();
    m_statusLabel->setText(QStringLiteral("Cancelling scan..."));
}

void MainWindow::onScanStarted(const QString& rootPath) {
    m_btnScan->setEnabled(false);
    m_btnCancel->setEnabled(true);
    m_btnBrowse->setEnabled(false);
    m_driveCombo->setEnabled(false);

    m_progressBar->setRange(0, 0); // Indeterminate pulsing
    m_statusLabel->setText(QStringLiteral("Scanning: ") + rootPath);
    m_statsLabel->setText(QStringLiteral("Files: 0 | Total: 0 B"));
}

void MainWindow::onScanProgress(quint64 files, quint64 bytes, const QString& currentFolder) {
    m_statsLabel->setText(QStringLiteral("Files: %1 | Total: %2")
        .arg(files)
        .arg(DiskNode::formatSize(bytes)));

    QFontMetrics fm(m_statusLabel->font());
    QString elidedPath = fm.elidedText(currentFolder, Qt::ElideMiddle, 380);
    m_statusLabel->setText(QStringLiteral("Scanning: ") + elidedPath);
}

void MainWindow::onScanFinished(std::shared_ptr<DiskNode> rootNode, qint64 elapsedMs, bool wasCancelled) {
    m_btnScan->setEnabled(true);
    m_btnCancel->setEnabled(false);
    m_btnBrowse->setEnabled(true);
    m_driveCombo->setEnabled(true);

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);

    double sec = elapsedMs / 1000.0;
    m_timeLabel->setText(QString::asprintf("Time: %.1fs", sec));

    if (wasCancelled) {
        m_statusLabel->setText(QStringLiteral("Scan cancelled by user. Partial results displayed."));
    } else {
        m_statusLabel->setText(QStringLiteral("Scan complete!"));
    }

    m_rootNode = rootNode;
    if (m_rootNode) {
        m_statsLabel->setText(QStringLiteral("Files: %1 | Total: %2")
            .arg(m_rootNode->fileCount())
            .arg(DiskNode::formatSize(m_rootNode->size())));

        // Populate tree model
        m_treeModel->setRootNode(m_rootNode.get());
        if (m_treeModel->rowCount() > 0) {
            m_treeView->expand(m_treeModel->index(0, 0));
        }

        // Populate treemap and sunburst
        m_treemapWidget->setRootNode(m_rootNode.get());
        m_sunburstWidget->setRootNode(m_rootNode.get());

        // Populate analytics tabs
        m_extStatsWidget->populateFromNode(m_rootNode.get());
        m_topFilesWidget->populateFromNode(m_rootNode.get(), 100);

        // Update breadcrumb
        m_breadcrumb->setRootAndCurrent(m_rootNode.get(), m_rootNode.get());
    } else {
        m_treeModel->setRootNode(nullptr);
        m_treemapWidget->setRootNode(nullptr);
        m_sunburstWidget->setRootNode(nullptr);
        m_extStatsWidget->clear();
        m_topFilesWidget->clear();
        m_breadcrumb->setRootAndCurrent(nullptr, nullptr);
    }
}

void MainWindow::onScanError(const QString& message) {
    QMessageBox::critical(this, QStringLiteral("Scan Error"), message);
}

void MainWindow::setVisualizerView(int index) {
    m_visStack->setCurrentIndex(index);
    m_btnTreemapView->setChecked(index == 0);
    m_btnSunburstView->setChecked(index == 1);

    // Synchronize roots and selections between visualizers
    if (index == 0 && m_sunburstWidget->currentRoot()) {
        m_treemapWidget->zoomIn(m_sunburstWidget->currentRoot());
        if (m_sunburstWidget->selectedNode()) {
            m_treemapWidget->selectNode(m_sunburstWidget->selectedNode());
        }
    } else if (index == 1 && m_treemapWidget->currentRoot()) {
        m_sunburstWidget->zoomIn(m_treemapWidget->currentRoot());
        if (m_treemapWidget->selectedNode()) {
            m_sunburstWidget->selectNode(m_treemapWidget->selectedNode());
        }
    }
}

void MainWindow::onTreeSelectionChanged(const QModelIndex& current, const QModelIndex& /*previous*/) {
    if (!current.isValid()) return;
    DiskNode* node = m_treeModel->nodeForIndex(current);
    if (node) {
        m_treemapWidget->selectNode(node);
        m_sunburstWidget->selectNode(node);
    }
}

void MainWindow::onTreemapNodeSelected(DiskNode* node) {
    if (!node) return;
    m_sunburstWidget->selectNode(node);
    QModelIndex idx = m_treeModel->indexForNode(node);
    if (idx.isValid()) {
        m_treeView->setCurrentIndex(idx);
        m_treeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void MainWindow::onTreemapNodeDoubleClicked(DiskNode* node) {
    if (!node) return;
    if (node->isDirectory()) {
        m_sunburstWidget->zoomIn(node);
        m_breadcrumb->setRootAndCurrent(m_rootNode.get(), node);
    }
}

void MainWindow::onTreemapRootChanged(DiskNode* newRoot) {
    if (m_rootNode && newRoot) {
        if (m_sunburstWidget->currentRoot() != newRoot) {
            m_sunburstWidget->zoomIn(newRoot);
        }
        m_breadcrumb->setRootAndCurrent(m_rootNode.get(), newRoot);
    }
}

void MainWindow::onSunburstNodeSelected(DiskNode* node) {
    if (!node) return;
    m_treemapWidget->selectNode(node);
    QModelIndex idx = m_treeModel->indexForNode(node);
    if (idx.isValid()) {
        m_treeView->setCurrentIndex(idx);
        m_treeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}

void MainWindow::onSunburstNodeDoubleClicked(DiskNode* node) {
    if (!node) return;
    if (node->isDirectory()) {
        m_treemapWidget->zoomIn(node);
        m_breadcrumb->setRootAndCurrent(m_rootNode.get(), node);
    }
}

void MainWindow::onSunburstRootChanged(DiskNode* newRoot) {
    if (m_rootNode && newRoot) {
        if (m_treemapWidget->currentRoot() != newRoot) {
            m_treemapWidget->zoomIn(newRoot);
        }
        m_breadcrumb->setRootAndCurrent(m_rootNode.get(), newRoot);
    }
}

void MainWindow::onBreadcrumbFolderSelected(DiskNode* node) {
    if (node) {
        m_treemapWidget->zoomIn(node);
        m_sunburstWidget->zoomIn(node);
    }
}

void MainWindow::onBreadcrumbUpRequested() {
    m_treemapWidget->zoomOut();
    m_sunburstWidget->zoomOut();
}

void MainWindow::onBreadcrumbResetRequested() {
    if (m_rootNode) {
        m_treemapWidget->zoomIn(m_rootNode.get());
        m_sunburstWidget->zoomIn(m_rootNode.get());
    }
}

void MainWindow::onTopFileSelected(DiskNode* node) {
    if (!node) return;
    m_tabs->setCurrentIndex(0); // Switch to TreeView tab
    QModelIndex idx = m_treeModel->indexForNode(node);
    if (idx.isValid()) {
        m_treeView->setCurrentIndex(idx);
        m_treeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
    m_treemapWidget->selectNode(node);
    m_sunburstWidget->selectNode(node);
}

void MainWindow::onExportHtmlClicked() {
    if (!m_rootNode) {
        QMessageBox::warning(this, QStringLiteral("No Scan Data"), QStringLiteral("Please complete a scan before exporting an interactive HTML report."));
        return;
    }

    QString defaultName = QStringLiteral("MemMap_Report_%1.html").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Export Interactive HTML Report"), defaultName, QStringLiteral("HTML Files (*.html)"));
    if (filePath.isEmpty()) return;

    QString errorMsg;
    if (ReportExporter::exportToHtml(m_rootNode.get(), filePath, &errorMsg)) {
        auto reply = QMessageBox::information(this, QStringLiteral("Export Succeeded"),
            QStringLiteral("Interactive HTML report successfully generated at:\n%1\n\nWould you like to open it in your browser now?").arg(filePath),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        }
    } else {
        QMessageBox::critical(this, QStringLiteral("Export Failed"), errorMsg);
    }
}

void MainWindow::onExportCsvClicked() {
    if (!m_rootNode) {
        QMessageBox::warning(this, QStringLiteral("No Scan Data"), QStringLiteral("Please complete a scan before exporting a CSV spreadsheet."));
        return;
    }

    QString defaultName = QStringLiteral("MemMap_Scan_%1.csv").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Export CSV Spreadsheet"), defaultName, QStringLiteral("CSV Files (*.csv)"));
    if (filePath.isEmpty()) return;

    QString errorMsg;
    if (ReportExporter::exportToCsv(m_rootNode.get(), filePath, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("Export Succeeded"),
            QStringLiteral("Scan data successfully exported to CSV:\n%1").arg(filePath));
    } else {
        QMessageBox::critical(this, QStringLiteral("Export Failed"), errorMsg);
    }
}

void MainWindow::onExportJsonClicked() {
    if (!m_rootNode) {
        QMessageBox::warning(this, QStringLiteral("No Scan Data"), QStringLiteral("Please complete a scan before exporting JSON data."));
        return;
    }

    QString defaultName = QStringLiteral("MemMap_Scan_%1.json").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Export JSON Data"), defaultName, QStringLiteral("JSON Files (*.json)"));
    if (filePath.isEmpty()) return;

    QString errorMsg;
    if (ReportExporter::exportToJson(m_rootNode.get(), filePath, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("Export Succeeded"),
            QStringLiteral("Scan tree successfully exported to JSON:\n%1").arg(filePath));
    } else {
        QMessageBox::critical(this, QStringLiteral("Export Failed"), errorMsg);
    }
}

void MainWindow::onSaveSnapshotClicked() {
    if (!m_rootNode) {
        QMessageBox::warning(this, QStringLiteral("No Scan Data"), QStringLiteral("Please complete a scan before saving a snapshot."));
        return;
    }

    QString defaultName = QStringLiteral("snapshot_%1.mmap").arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    QString filePath = QFileDialog::getSaveFileName(this, QStringLiteral("Save Scan Snapshot"), defaultName, QStringLiteral("Mem-Map Snapshots (*.mmap);;All Files (*.*)"));
    if (filePath.isEmpty()) return;

    QString errorMsg;
    if (SnapshotEngine::saveSnapshot(m_rootNode.get(), filePath, &errorMsg)) {
        QMessageBox::information(this, QStringLiteral("Snapshot Saved"),
            QStringLiteral("Scan snapshot successfully saved to:\n%1").arg(filePath));
    } else {
        QMessageBox::critical(this, QStringLiteral("Save Failed"), errorMsg);
    }
}

void MainWindow::onCompareWithSnapshotClicked() {
    if (!m_rootNode) {
        QMessageBox::warning(this, QStringLiteral("No Active Scan"), QStringLiteral("Please scan a directory or drive first to compare it against a baseline snapshot."));
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("Select Baseline Snapshot to Compare Against"), QString(), QStringLiteral("Mem-Map Snapshots (*.mmap);;All Files (*.*)"));
    if (filePath.isEmpty()) return;

    QString errorMsg;
    auto baselineRoot = SnapshotEngine::loadSnapshot(filePath, &errorMsg);
    if (!baselineRoot) {
        QMessageBox::critical(this, QStringLiteral("Failed to Load Snapshot"), errorMsg);
        return;
    }

    DiffSummary summary;
    auto diffRoot = SnapshotEngine::compareTrees(baselineRoot.get(), m_rootNode.get(), summary);
    if (!diffRoot) {
        QMessageBox::warning(this, QStringLiteral("Comparison Error"), QStringLiteral("Could not compare the selected snapshot with the current scan."));
        return;
    }

    m_snapshotDiffWidget->setDiffData(std::move(diffRoot), summary);
    m_tabs->setCurrentWidget(m_snapshotDiffWidget);
}

void MainWindow::onCompareTwoSnapshotsClicked() {
    QString file1 = QFileDialog::getOpenFileName(this, QStringLiteral("Select Baseline (Old) Snapshot"), QString(), QStringLiteral("Mem-Map Snapshots (*.mmap);;All Files (*.*)"));
    if (file1.isEmpty()) return;

    QString file2 = QFileDialog::getOpenFileName(this, QStringLiteral("Select New Snapshot"), QString(), QStringLiteral("Mem-Map Snapshots (*.mmap);;All Files (*.*)"));
    if (file2.isEmpty()) return;

    QString errorMsg;
    auto oldRoot = SnapshotEngine::loadSnapshot(file1, &errorMsg);
    if (!oldRoot) {
        QMessageBox::critical(this, QStringLiteral("Failed to Load Baseline Snapshot"), errorMsg);
        return;
    }

    auto newRoot = SnapshotEngine::loadSnapshot(file2, &errorMsg);
    if (!newRoot) {
        QMessageBox::critical(this, QStringLiteral("Failed to Load New Snapshot"), errorMsg);
        return;
    }

    DiffSummary summary;
    auto diffRoot = SnapshotEngine::compareTrees(oldRoot.get(), newRoot.get(), summary);
    if (!diffRoot) {
        QMessageBox::warning(this, QStringLiteral("Comparison Error"), QStringLiteral("Could not compare the snapshots."));
        return;
    }

    m_snapshotDiffWidget->setDiffData(std::move(diffRoot), summary);
    m_tabs->setCurrentWidget(m_snapshotDiffWidget);
}
