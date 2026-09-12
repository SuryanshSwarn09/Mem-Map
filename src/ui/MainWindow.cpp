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
#include "AboutDialog.h"
#include "ThemeManager.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_scanner(new ScannerEngine(this))
{
    setupUi();
    applyDarkTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](ThemePreset) {
        applyDarkTheme();
    });
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

    // Global Keyboard Shortcut: Ctrl+S to save disk snapshot
    QShortcut* shortcutCtrlS = new QShortcut(QKeySequence::Save, this);
    connect(shortcutCtrlS, &QShortcut::activated, this, [this]() {
        if (m_btnSnapshot && m_btnSnapshot->isEnabled()) {
            onSaveSnapshotClicked();
        }
    });

    // Global Keyboard Shortcut: Ctrl+E to export HTML report
    QShortcut* shortcutCtrlE = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this);
    connect(shortcutCtrlE, &QShortcut::activated, this, [this]() {
        if (m_btnExport && m_btnExport->isEnabled()) {
            onExportHtmlClicked();
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

    // Top Bar 1: Drive selection and actions inside a sleek header container
    QFrame* topBarCard = new QFrame(this);
    topBarCard->setObjectName(QStringLiteral("headerBar"));
    QHBoxLayout* topBar = new QHBoxLayout(topBarCard);
    topBar->setContentsMargins(10, 8, 10, 8);
    topBar->setSpacing(8);

    QLabel* lblDrive = new QLabel(QStringLiteral("TARGET"), this);
    lblDrive->setStyleSheet(QStringLiteral("font-weight: 700; font-size: 10px; color: #8B949E; letter-spacing: 1px; padding: 0 4px;"));

    m_driveCombo = new QComboBox(this);
    m_driveCombo->setMinimumWidth(230);
    connect(m_driveCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDriveSelected);

    m_btnBrowse = new QPushButton(QStringLiteral("📂 Browse..."), this);
    m_btnBrowse->setToolTip(QStringLiteral("Browse folder to scan (Ctrl+O)"));
    connect(m_btnBrowse, &QPushButton::clicked, this, &MainWindow::onSelectFolderClicked);

    m_btnScan = new QPushButton(QStringLiteral("⚡ Scan Now"), this);
    m_btnScan->setObjectName(QStringLiteral("primaryScanBtn"));
    m_btnScan->setToolTip(QStringLiteral("Start scanning selected target (F5)"));
    connect(m_btnScan, &QPushButton::clicked, this, &MainWindow::onStartScanClicked);

    m_btnCancel = new QPushButton(QStringLiteral("✖ Cancel"), this);
    m_btnCancel->setObjectName(QStringLiteral("cancelScanBtn"));
    m_btnCancel->setToolTip(QStringLiteral("Cancel ongoing scan"));
    m_btnCancel->setEnabled(false);
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancelScanClicked);

    topBar->addWidget(lblDrive);
    topBar->addWidget(m_driveCombo);
    topBar->addWidget(m_btnBrowse);
    topBar->addWidget(m_btnScan);
    topBar->addWidget(m_btnCancel);

    // Subtle divider
    QFrame* divider1 = new QFrame(this);
    divider1->setFrameShape(QFrame::VLine);
    divider1->setStyleSheet(QStringLiteral("color: #30363D; margin: 4px 2px;"));
    topBar->addWidget(divider1);

    // Export button with dropdown menu
    m_btnExport = new QPushButton(QStringLiteral("📑 Reports ▾"), this);
    m_btnExport->setToolTip(QStringLiteral("Export scan report or raw data (Ctrl+E for HTML)"));
    QMenu* exportMenu = new QMenu(m_btnExport);
    QAction* actHtml = exportMenu->addAction(QStringLiteral("🌐 Interactive HTML Report (.html)..."));
    actHtml->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    QAction* actCsv  = exportMenu->addAction(QStringLiteral("📊 Spreadsheet Data (.csv)..."));
    QAction* actJson = exportMenu->addAction(QStringLiteral("📄 Raw Tree Data (.json)..."));
    m_btnExport->setMenu(exportMenu);
    connect(actHtml, &QAction::triggered, this, &MainWindow::onExportHtmlClicked);
    connect(actCsv, &QAction::triggered, this, &MainWindow::onExportCsvClicked);
    connect(actJson, &QAction::triggered, this, &MainWindow::onExportJsonClicked);
    topBar->addWidget(m_btnExport);

    // Snapshot button with dropdown menu
    m_btnSnapshot = new QPushButton(QStringLiteral("💾 Snapshots ▾"), this);
    m_btnSnapshot->setToolTip(QStringLiteral("Save or compare disk snapshots (Ctrl+S to save)"));
    QMenu* snapshotMenu = new QMenu(m_btnSnapshot);
    QAction* actSaveSnap = snapshotMenu->addAction(QStringLiteral("💾 Save Current Scan as Snapshot (.mmap)..."));
    actSaveSnap->setShortcut(QKeySequence::Save);
    snapshotMenu->addSeparator();
    QAction* actCompareWith = snapshotMenu->addAction(QStringLiteral("⚖ Compare Current Scan with Snapshot..."));
    QAction* actCompareTwo  = snapshotMenu->addAction(QStringLiteral("🔄 Compare Two Saved Snapshots..."));
    m_btnSnapshot->setMenu(snapshotMenu);
    connect(actSaveSnap, &QAction::triggered, this, &MainWindow::onSaveSnapshotClicked);
    connect(actCompareWith, &QAction::triggered, this, &MainWindow::onCompareWithSnapshotClicked);
    connect(actCompareTwo, &QAction::triggered, this, &MainWindow::onCompareTwoSnapshotsClicked);
    topBar->addWidget(m_btnSnapshot);

    // Creator & Social Links button
    m_btnAbout = new QPushButton(QStringLiteral("👤 Creator"), this);
    m_btnAbout->setToolTip(QStringLiteral("About Mem-Map, creator profile (Suryansh Swarn), and social links"));
    connect(m_btnAbout, &QPushButton::clicked, this, &MainWindow::onAboutClicked);
    topBar->addWidget(m_btnAbout);

    topBar->addStretch();

    // Theme Preset Selector Pill
    m_themeCombo = new QComboBox(this);
    m_themeCombo->setToolTip(QStringLiteral("Switch Visual Theme Preset"));
    m_themeCombo->addItem(QStringLiteral("🌙 Midnight Slate"), static_cast<int>(ThemePreset::MidnightSlate));
    m_themeCombo->addItem(QStringLiteral("🖤 Obsidian OLED"), static_cast<int>(ThemePreset::ObsidianOLED));
    m_themeCombo->addItem(QStringLiteral("❄️ Nordic Frost"), static_cast<int>(ThemePreset::NordicFrost));
    connect(m_themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        ThemePreset preset = static_cast<ThemePreset>(m_themeCombo->currentData().toInt());
        ThemeManager::instance().setTheme(preset);
    });
    topBar->addWidget(m_themeCombo);

    rootLayout->addWidget(topBarCard);

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

    // Tab 5: Duplicate File Finder
    m_duplicateWidget = new DuplicateFilesWidget(this);
    m_tabs->addTab(m_duplicateWidget, QStringLiteral("Duplicate Files"));

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
    visHeader->addSpacing(12);

    QLabel* lblColor = new QLabel(QStringLiteral("Color:"), this);
    lblColor->setStyleSheet(QStringLiteral("color: #8B949E; font-weight: bold; font-size: 12px;"));
    visHeader->addWidget(lblColor);

    m_btnColorMode = new QPushButton(QStringLiteral("🎨 File Type"), this);
    m_btnColorMode->setCheckable(true);
    m_btnColorMode->setObjectName(QStringLiteral("visToggle"));
    m_btnColorMode->setToolTip(QStringLiteral("Toggle visualizer coloring between File Type and File Age Heatmap (Hot = Recent, Cold = Stale)"));
    connect(m_btnColorMode, &QPushButton::clicked, this, &MainWindow::toggleColorMode);
    visHeader->addWidget(m_btnColorMode);

    visHeader->addStretch();
    visLayout->addLayout(visHeader);

    // Heatmap Color Legend Banner (visible when File Age mode is active)
    m_heatmapLegend = new QWidget(this);
    m_heatmapLegend->setStyleSheet(QStringLiteral(
        "background-color: #161B22; border: 1px solid #30363D; border-radius: 4px; padding: 2px;"
    ));
    QHBoxLayout* legendLayout = new QHBoxLayout(m_heatmapLegend);
    legendLayout->setContentsMargins(8, 3, 8, 3);
    legendLayout->setSpacing(10);

    QLabel* lblAgeTitle = new QLabel(QStringLiteral("<b>Age Scale:</b>"), m_heatmapLegend);
    lblAgeTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; border: none;"));
    legendLayout->addWidget(lblAgeTitle);

    auto makeSwatch = [this](const QString& colorHex, const QString& text) {
        QLabel* lbl = new QLabel(QStringLiteral("<span style='color:%1;'>■</span> <span style='color:#C9D1D9;'>%2</span>").arg(colorHex, text), m_heatmapLegend);
        lbl->setStyleSheet(QStringLiteral("font-size: 11px; border: none;"));
        return lbl;
    };

    legendLayout->addWidget(makeSwatch(QStringLiteral("#F85149"), QStringLiteral("&lt; 7d (Hot)")));
    legendLayout->addWidget(makeSwatch(QStringLiteral("#D29922"), QStringLiteral("&lt; 1mo")));
    legendLayout->addWidget(makeSwatch(QStringLiteral("#3FB950"), QStringLiteral("&lt; 6mo")));
    legendLayout->addWidget(makeSwatch(QStringLiteral("#388BFD"), QStringLiteral("&lt; 1yr")));
    legendLayout->addWidget(makeSwatch(QStringLiteral("#6E7681"), QStringLiteral("1-2yr")));
    legendLayout->addWidget(makeSwatch(QStringLiteral("#30363D"), QStringLiteral("&gt; 2yr (Cold)")));
    legendLayout->addStretch();

    m_heatmapLegend->setVisible(false);
    visLayout->addWidget(m_heatmapLegend);

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
    setStyleSheet(ThemeManager::instance().generateApplicationStyleSheet());
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
        m_duplicateWidget->setRootNode(m_rootNode.get());

        // Update breadcrumb
        m_breadcrumb->setRootAndCurrent(m_rootNode.get(), m_rootNode.get());
    } else {
        m_treeModel->setRootNode(nullptr);
        m_treemapWidget->setRootNode(nullptr);
        m_sunburstWidget->setRootNode(nullptr);
        m_extStatsWidget->clear();
        m_topFilesWidget->clear();
        m_duplicateWidget->clear();
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

void MainWindow::toggleColorMode() {
    bool isAge = m_btnColorMode->isChecked();
    if (isAge) {
        m_btnColorMode->setText(QStringLiteral("🌡 File Age (Heatmap)"));
        m_treemapWidget->setColorMode(TreemapWidget::ColorMode::FileAge);
        m_sunburstWidget->setColorMode(SunburstWidget::ColorMode::FileAge);
        m_heatmapLegend->setVisible(true);
    } else {
        m_btnColorMode->setText(QStringLiteral("🎨 File Type"));
        m_treemapWidget->setColorMode(TreemapWidget::ColorMode::FileType);
        m_sunburstWidget->setColorMode(SunburstWidget::ColorMode::FileType);
        m_heatmapLegend->setVisible(false);
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

void MainWindow::onAboutClicked() {
    AboutDialog dlg(this);
    dlg.exec();
}
