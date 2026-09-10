#include "DuplicateFilesWidget.h"
#include "core/DiskNode.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QHeaderView>
#include <QFileInfo>
#include <QDateTime>
#include <QFileIconProvider>

DuplicateFilesWidget::DuplicateFilesWidget(QWidget* parent)
    : QWidget(parent)
{
    m_finder = new DuplicateFinder(this);
    connect(m_finder, &DuplicateFinder::scanStarted, this, &DuplicateFilesWidget::onScanStarted);
    connect(m_finder, &DuplicateFinder::scanFinished, this, &DuplicateFilesWidget::onScanFinished);
    connect(m_finder, &DuplicateFinder::scanCancelled, this, &DuplicateFilesWidget::onScanCancelled);

    setupUi();
}

DuplicateFilesWidget::~DuplicateFilesWidget() {
    if (m_finder && m_finder->isRunning()) {
        m_finder->cancelScan();
        m_finder->wait();
    }
}

void DuplicateFilesWidget::setRootNode(DiskNode* rootNode) {
    m_rootNode = rootNode;
    clear();
    if (m_statusLabel) {
        if (m_rootNode) {
            m_statusLabel->setText(QStringLiteral("Ready to scan root: %1").arg(m_rootNode->fullPath()));
        } else {
            m_statusLabel->setText(QStringLiteral("No directory loaded. Please scan a drive or folder first."));
        }
    }
}

void DuplicateFilesWidget::clear() {
    m_result = DuplicateScanResult();
    if (m_treeWidget) {
        m_treeWidget->clear();
    }
    updateKpiDisplay();
    updateSelectedStats();
}

void DuplicateFilesWidget::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    createKpiHeader();
    createActionToolbar();
    createResultsTree();
}

void DuplicateFilesWidget::createKpiHeader() {
    QFrame* banner = new QFrame(this);
    banner->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #161B22; border: 1px solid #30363D; border-radius: 8px; }"
    ));
    QHBoxLayout* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(16, 12, 16, 12);
    bannerLayout->setSpacing(24);

    // Card 1: Total Wasted Space (Highlight in coral #F85149)
    QVBoxLayout* wastedLayout = new QVBoxLayout();
    QLabel* wastedTitle = new QLabel(QStringLiteral("TOTAL WASTED SPACE"), banner);
    wastedTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; border: none;"));
    m_wastedSpaceLabel = new QLabel(QStringLiteral("0 B"), banner);
    m_wastedSpaceLabel->setStyleSheet(QStringLiteral("color: #F85149; font-size: 22px; font-weight: bold; border: none;"));
    wastedLayout->addWidget(wastedTitle);
    wastedLayout->addWidget(m_wastedSpaceLabel);
    bannerLayout->addLayout(wastedLayout, 1);

    // Card 2: Duplicate Groups
    QVBoxLayout* groupsLayout = new QVBoxLayout();
    QLabel* groupsTitle = new QLabel(QStringLiteral("DUPLICATE GROUPS"), banner);
    groupsTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; border: none;"));
    m_duplicateGroupsLabel = new QLabel(QStringLiteral("0 groups"), banner);
    m_duplicateGroupsLabel->setStyleSheet(QStringLiteral("color: #C9D1D9; font-size: 18px; font-weight: bold; border: none;"));
    groupsLayout->addWidget(groupsTitle);
    groupsLayout->addWidget(m_duplicateGroupsLabel);
    bannerLayout->addLayout(groupsLayout, 1);

    // Card 3: Duplicate Files
    QVBoxLayout* filesLayout = new QVBoxLayout();
    QLabel* filesTitle = new QLabel(QStringLiteral("DUPLICATE FILES"), banner);
    filesTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; border: none;"));
    m_duplicateFilesLabel = new QLabel(QStringLiteral("0 files"), banner);
    m_duplicateFilesLabel->setStyleSheet(QStringLiteral("color: #C9D1D9; font-size: 18px; font-weight: bold; border: none;"));
    filesLayout->addWidget(filesTitle);
    filesLayout->addWidget(m_duplicateFilesLabel);
    bannerLayout->addLayout(filesLayout, 1);

    // Card 4: Selected for Cleanup
    QVBoxLayout* selectedLayout = new QVBoxLayout();
    QLabel* selectedTitle = new QLabel(QStringLiteral("SELECTED FOR CLEANUP"), banner);
    selectedTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; border: none;"));
    m_selectedCountLabel = new QLabel(QStringLiteral("0 files (0 B)"), banner);
    m_selectedCountLabel->setStyleSheet(QStringLiteral("color: #58A6FF; font-size: 18px; font-weight: bold; border: none;"));
    selectedLayout->addWidget(selectedTitle);
    selectedLayout->addWidget(m_selectedCountLabel);
    bannerLayout->addLayout(selectedLayout, 1);

    layout()->addWidget(banner);
}

void DuplicateFilesWidget::updateKpiDisplay() {
    if (!m_wastedSpaceLabel) return;

    m_wastedSpaceLabel->setText(DiskNode::formatSize(static_cast<uint64_t>(m_result.totalWastedBytes)));
    m_duplicateGroupsLabel->setText(QStringLiteral("%1 groups").arg(m_result.totalGroups));
    m_duplicateFilesLabel->setText(QStringLiteral("%1 files").arg(m_result.totalDuplicateFiles));
}

void DuplicateFilesWidget::createActionToolbar() {
    QWidget* toolbar = new QWidget(this);
    QVBoxLayout* toolVBox = new QVBoxLayout(toolbar);
    toolVBox->setContentsMargins(0, 0, 0, 0);
    toolVBox->setSpacing(8);

    // Row 1: Primary Controls
    QHBoxLayout* row1 = new QHBoxLayout();
    row1->setSpacing(8);

    m_scanBtn = new QPushButton(QStringLiteral("Find Duplicates"), toolbar);
    m_scanBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #238636; color: #FFFFFF; font-weight: bold; padding: 6px 14px; border-radius: 6px; border: 1px solid #2EA043; }"
        "QPushButton:hover { background-color: #2EA043; }"
        "QPushButton:pressed { background-color: #1F7F32; }"
        "QPushButton:disabled { background-color: #21262D; color: #484F58; border-color: #30363D; }"
    ));
    connect(m_scanBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::startScan);
    row1->addWidget(m_scanBtn);

    m_cancelBtn = new QPushButton(QStringLiteral("Cancel"), toolbar);
    m_cancelBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #F85149; font-weight: bold; padding: 6px 14px; border-radius: 6px; border: 1px solid #30363D; }"
        "QPushButton:hover { background-color: #30363D; }"
        "QPushButton:disabled { color: #484F58; border-color: #21262D; }"
    ));
    m_cancelBtn->setEnabled(false);
    connect(m_cancelBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::cancelScan);
    row1->addWidget(m_cancelBtn);

    QLabel* algoLabel = new QLabel(QStringLiteral("Algorithm:"), toolbar);
    algoLabel->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 12px;"));
    row1->addWidget(algoLabel);

    m_algoCombo = new QComboBox(toolbar);
    m_algoCombo->addItem(QStringLiteral("MD5 (Ultra Fast)"), static_cast<int>(HashAlgorithm::Md5));
    m_algoCombo->addItem(QStringLiteral("SHA-256 (Cryptographic)"), static_cast<int>(HashAlgorithm::Sha256));
    m_algoCombo->setStyleSheet(QStringLiteral(
        "QComboBox { background-color: #161B22; color: #C9D1D9; border: 1px solid #30363D; border-radius: 6px; padding: 4px 10px; font-size: 12px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #161B22; color: #C9D1D9; selection-background-color: #1F6FEB; }"
    ));
    row1->addWidget(m_algoCombo);

    row1->addSpacing(12);

    // Smart auto-selection buttons
    m_keepNewestBtn = new QPushButton(QStringLiteral("Keep Newest"), toolbar);
    m_keepNewestBtn->setToolTip(QStringLiteral("In each duplicate group, select older copies for deletion and keep the newest."));
    m_keepNewestBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #C9D1D9; padding: 5px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; color: #58A6FF; }"
    ));
    connect(m_keepNewestBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::selectKeepNewest);
    row1->addWidget(m_keepNewestBtn);

    m_keepOldestBtn = new QPushButton(QStringLiteral("Keep Oldest"), toolbar);
    m_keepOldestBtn->setToolTip(QStringLiteral("In each duplicate group, select newer copies for deletion and keep the oldest."));
    m_keepOldestBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #C9D1D9; padding: 5px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; color: #58A6FF; }"
    ));
    connect(m_keepOldestBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::selectKeepOldest);
    row1->addWidget(m_keepOldestBtn);

    m_selectAllBtn = new QPushButton(QStringLiteral("Select All"), toolbar);
    m_selectAllBtn->setToolTip(QStringLiteral("Select all duplicate copies (keeping the first copy untouched)."));
    m_selectAllBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #C9D1D9; padding: 5px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; color: #58A6FF; }"
    ));
    connect(m_selectAllBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::selectAllDuplicates);
    row1->addWidget(m_selectAllBtn);

    m_deselectAllBtn = new QPushButton(QStringLiteral("Deselect All"), toolbar);
    m_deselectAllBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #21262D; color: #C9D1D9; padding: 5px 10px; border-radius: 6px; border: 1px solid #30363D; font-size: 12px; }"
        "QPushButton:hover { background-color: #30363D; }"
    ));
    connect(m_deselectAllBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::deselectAll);
    row1->addWidget(m_deselectAllBtn);

    row1->addStretch();

    // Delete selected button
    m_deleteBtn = new QPushButton(QStringLiteral("Move Selected to Recycle Bin"), toolbar);
    m_deleteBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #DA3633; color: #FFFFFF; font-weight: bold; padding: 6px 14px; border-radius: 6px; border: 1px solid #F85149; font-size: 12px; }"
        "QPushButton:hover { background-color: #F85149; }"
        "QPushButton:pressed { background-color: #B62324; }"
        "QPushButton:disabled { background-color: #21262D; color: #484F58; border-color: #30363D; }"
    ));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &DuplicateFilesWidget::deleteSelectedToTrash);
    row1->addWidget(m_deleteBtn);

    toolVBox->addLayout(row1);

    // Row 2: Search filter & status
    QHBoxLayout* row2 = new QHBoxLayout();
    row2->setSpacing(10);

    m_searchEdit = new QLineEdit(toolbar);
    m_searchEdit->setPlaceholderText(QStringLiteral("Filter duplicates by file name or path..."));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setStyleSheet(QStringLiteral(
        "QLineEdit { background-color: #0D1117; color: #C9D1D9; border: 1px solid #30363D; border-radius: 6px; padding: 5px 10px; font-size: 12px; }"
        "QLineEdit:focus { border: 1px solid #58A6FF; }"
    ));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &DuplicateFilesWidget::filterFiles);
    row2->addWidget(m_searchEdit, 2);

    m_statusLabel = new QLabel(QStringLiteral("Ready to scan."), toolbar);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 12px;"));
    row2->addWidget(m_statusLabel, 3);

    toolVBox->addLayout(row2);

    // Live scan progress bar
    m_progressBar = new QProgressBar(toolbar);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(3);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background-color: #21262D; border: none; border-radius: 1px; }"
        "QProgressBar::chunk { background-color: #238636; }"
    ));
    m_progressBar->hide();
    toolVBox->addWidget(m_progressBar);

    layout()->addWidget(toolbar);
}

void DuplicateFilesWidget::startScan() {
    if (!m_rootNode) {
        if (m_statusLabel) {
            m_statusLabel->setText(QStringLiteral("Please load or scan a directory first."));
        }
        return;
    }

    HashAlgorithm algo = HashAlgorithm::Md5;
    if (m_algoCombo && m_algoCombo->currentData().toInt() == static_cast<int>(HashAlgorithm::Sha256)) {
        algo = HashAlgorithm::Sha256;
    }

    m_finder->startDuplicateScan(m_rootNode, algo);
}

void DuplicateFilesWidget::cancelScan() {
    if (m_finder && m_finder->isRunning()) {
        m_finder->cancelScan();
        m_statusLabel->setText(QStringLiteral("Cancelling duplicate scan..."));
    }
}

void DuplicateFilesWidget::onScanStarted() {
    m_scanBtn->setEnabled(false);
    m_cancelBtn->setEnabled(true);
    m_progressBar->show();
    m_statusLabel->setText(QStringLiteral("Scanning file hierarchy and comparing byte-level hashes..."));
}

void DuplicateFilesWidget::onScanFinished(const DuplicateScanResult& result) {
    m_result = result;
    m_scanBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_progressBar->hide();

    updateKpiDisplay();
    populateTree();

    m_statusLabel->setText(QStringLiteral("Scan completed in %1 ms. Found %2 groups (%3 duplicate files) wasting %4.")
        .arg(m_result.elapsedMs)
        .arg(m_result.totalGroups)
        .arg(m_result.totalDuplicateFiles)
        .arg(DiskNode::formatSize(static_cast<uint64_t>(m_result.totalWastedBytes))));
}

void DuplicateFilesWidget::onScanCancelled() {
    m_scanBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_progressBar->hide();
    m_statusLabel->setText(QStringLiteral("Duplicate scan cancelled by user."));
}

void DuplicateFilesWidget::createResultsTree() {
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels({
        QStringLiteral("Duplicate File / Group"),
        QStringLiteral("Full Path"),
        QStringLiteral("Size"),
        QStringLiteral("Last Modified")
    });
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &DuplicateFilesWidget::showContextMenu);
    connect(m_treeWidget, &QTreeWidget::itemChanged, this, &DuplicateFilesWidget::onItemChanged);

    // Style Tree Widget for GitHub Dark / Mem-Map aesthetic
    m_treeWidget->setStyleSheet(QStringLiteral(
        "QTreeWidget { background-color: #0D1117; color: #C9D1D9; border: 1px solid #30363D; border-radius: 8px; alternate-background-color: #161B22; }"
        "QTreeWidget::item { padding: 4px 6px; border-bottom: 1px solid #21262D; }"
        "QTreeWidget::item:hover { background-color: #1F242C; }"
        "QTreeWidget::item:selected { background-color: #1F6FEB; color: #FFFFFF; }"
        "QHeaderView::section { background-color: #161B22; color: #8B949E; padding: 6px 10px; border: none; border-right: 1px solid #30363D; border-bottom: 1px solid #30363D; font-weight: bold; font-size: 11px; }"
    ));

    QHeaderView* header = m_treeWidget->header();
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    header->setSectionResizeMode(3, QHeaderView::Interactive);
    m_treeWidget->setColumnWidth(0, 320);
    m_treeWidget->setColumnWidth(2, 100);
    m_treeWidget->setColumnWidth(3, 160);

    layout()->addWidget(m_treeWidget, 1);
}

void DuplicateFilesWidget::populateTree() {
    m_isUpdatingCheckState = true;
    m_treeWidget->clear();

    QFileIconProvider iconProvider;

    for (size_t groupIdx = 0; groupIdx < m_result.groups.size(); ++groupIdx) {
        const auto& group = m_result.groups[groupIdx];
        QTreeWidgetItem* groupItem = new QTreeWidgetItem(m_treeWidget);
        
        QString shortHash = group.hash.left(8);
        QString groupTitle = QStringLiteral("Group %1: %2 duplicates | %3 each | Wasting %4 | [Hash: %5...]")
            .arg(groupIdx + 1)
            .arg(group.files.size())
            .arg(DiskNode::formatSize(group.fileSize))
            .arg(DiskNode::formatSize(group.wastedBytes()))
            .arg(shortHash);

        groupItem->setText(0, groupTitle);
        groupItem->setText(1, QStringLiteral("%1 matching files").arg(group.files.size()));
        groupItem->setText(2, DiskNode::formatSize(group.wastedBytes()));
        groupItem->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
        
        QFont boldFont = groupItem->font(0);
        boldFont.setBold(true);
        groupItem->setFont(0, boldFont);
        groupItem->setForeground(0, QBrush(QColor(QStringLiteral("#58A6FF"))));
        groupItem->setBackground(0, QBrush(QColor(QStringLiteral("#161B22"))));
        groupItem->setBackground(1, QBrush(QColor(QStringLiteral("#161B22"))));
        groupItem->setBackground(2, QBrush(QColor(QStringLiteral("#161B22"))));
        groupItem->setBackground(3, QBrush(QColor(QStringLiteral("#161B22"))));
        groupItem->setData(0, Qt::UserRole, static_cast<int>(groupIdx));

        for (size_t fileIdx = 0; fileIdx < group.files.size(); ++fileIdx) {
            const auto& df = group.files[fileIdx];
            QTreeWidgetItem* fileItem = new QTreeWidgetItem(groupItem);
            
            QFileInfo fi(df.path);
            fileItem->setIcon(0, iconProvider.icon(fi));
            fileItem->setText(0, fi.fileName());
            fileItem->setText(1, df.path);
            fileItem->setText(2, DiskNode::formatSize(df.size));
            fileItem->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
            fileItem->setText(3, QDateTime::fromSecsSinceEpoch(df.lastModified).toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
            
            fileItem->setFlags(fileItem->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            fileItem->setCheckState(0, df.isSelectedForDeletion ? Qt::Checked : Qt::Unchecked);

            fileItem->setData(0, Qt::UserRole, df.path);
            fileItem->setData(1, Qt::UserRole, static_cast<int>(groupIdx));
            fileItem->setData(2, Qt::UserRole, static_cast<int>(fileIdx));
        }

        groupItem->setExpanded(true);
    }

    m_isUpdatingCheckState = false;
    updateSelectedStats();
}

void DuplicateFilesWidget::filterFiles(const QString& query) {
    QString trimmed = query.trimmed();
    bool hasFilter = !trimmed.isEmpty();

    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* groupItem = m_treeWidget->topLevelItem(i);
        bool anyChildVisible = false;

        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem* child = groupItem->child(j);
            if (!hasFilter) {
                child->setHidden(false);
                anyChildVisible = true;
            } else {
                bool match = child->text(0).contains(trimmed, Qt::CaseInsensitive) ||
                             child->text(1).contains(trimmed, Qt::CaseInsensitive);
                child->setHidden(!match);
                if (match) anyChildVisible = true;
            }
        }

        groupItem->setHidden(!anyChildVisible);
        if (hasFilter && anyChildVisible) {
            groupItem->setExpanded(true);
        }
    }
}

// Scaffolds to be populated in upcoming micro-commits:
void DuplicateFilesWidget::updateSelectedStats() {}
void DuplicateFilesWidget::selectKeepNewest() {}
void DuplicateFilesWidget::selectKeepOldest() {}
void DuplicateFilesWidget::selectAllDuplicates() {}
void DuplicateFilesWidget::deselectAll() {}
void DuplicateFilesWidget::deleteSelectedToTrash() {}
void DuplicateFilesWidget::onItemChanged(QTreeWidgetItem* item, int column) { Q_UNUSED(item); Q_UNUSED(column); }
void DuplicateFilesWidget::showContextMenu(const QPoint& pos) { Q_UNUSED(pos); }

