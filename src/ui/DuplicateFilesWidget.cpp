#include "DuplicateFilesWidget.h"
#include "core/DiskNode.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QHeaderView>
#include <QFileInfo>
#include <QDateTime>
#include <QFileIconProvider>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QClipboard>
#include <QGuiApplication>
#include <QDir>
#include <QFile>

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

void DuplicateFilesWidget::onItemChanged(QTreeWidgetItem* item, int column) {
    if (m_isUpdatingCheckState || column != 0 || !item) return;

    // Check if this is a child file item
    if (item->parent() != nullptr) {
        int groupIdx = item->data(1, Qt::UserRole).toInt();
        int fileIdx = item->data(2, Qt::UserRole).toInt();

        if (groupIdx >= 0 && groupIdx < static_cast<int>(m_result.groups.size())) {
            auto& group = m_result.groups[groupIdx];
            if (fileIdx >= 0 && fileIdx < static_cast<int>(group.files.size())) {
                group.files[fileIdx].isSelectedForDeletion = (item->checkState(0) == Qt::Checked);
            }
        }
        updateSelectedStats();
    }
}

void DuplicateFilesWidget::updateSelectedStats() {
    int64_t selectedCount = 0;
    int64_t selectedBytes = 0;

    for (const auto& group : m_result.groups) {
        for (const auto& file : group.files) {
            if (file.isSelectedForDeletion) {
                selectedCount++;
                selectedBytes += file.size;
            }
        }
    }

    if (m_selectedCountLabel) {
        m_selectedCountLabel->setText(QStringLiteral("%1 files (%2)")
            .arg(selectedCount)
            .arg(DiskNode::formatSize(static_cast<uint64_t>(selectedBytes))));
    }

    if (m_deleteBtn) {
        m_deleteBtn->setEnabled(selectedCount > 0);
        if (selectedCount > 0) {
            m_deleteBtn->setText(QStringLiteral("Move %1 Selected to Recycle Bin (%2)")
                .arg(selectedCount)
                .arg(DiskNode::formatSize(static_cast<uint64_t>(selectedBytes))));
        } else {
            m_deleteBtn->setText(QStringLiteral("Move Selected to Recycle Bin"));
        }
    }
}

static void syncTreeCheckStates(QTreeWidget* treeWidget, const DuplicateScanResult& result) {
    if (!treeWidget) return;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* groupItem = treeWidget->topLevelItem(i);
        int groupIdx = groupItem->data(0, Qt::UserRole).toInt();
        if (groupIdx < 0 || groupIdx >= static_cast<int>(result.groups.size())) continue;

        const auto& group = result.groups[groupIdx];
        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem* child = groupItem->child(j);
            int fileIdx = child->data(2, Qt::UserRole).toInt();
            if (fileIdx >= 0 && fileIdx < static_cast<int>(group.files.size())) {
                child->setCheckState(0, group.files[fileIdx].isSelectedForDeletion ? Qt::Checked : Qt::Unchecked);
            }
        }
    }
}

void DuplicateFilesWidget::selectKeepNewest() {
    m_isUpdatingCheckState = true;
    for (auto& group : m_result.groups) {
        if (group.files.empty()) continue;
        size_t newestIdx = 0;
        int64_t newestTime = group.files[0].lastModified;
        for (size_t i = 1; i < group.files.size(); ++i) {
            if (group.files[i].lastModified > newestTime) {
                newestTime = group.files[i].lastModified;
                newestIdx = i;
            }
        }
        for (size_t i = 0; i < group.files.size(); ++i) {
            group.files[i].isSelectedForDeletion = (i != newestIdx);
        }
    }
    syncTreeCheckStates(m_treeWidget, m_result);
    m_isUpdatingCheckState = false;
    updateSelectedStats();
}

void DuplicateFilesWidget::selectKeepOldest() {
    m_isUpdatingCheckState = true;
    for (auto& group : m_result.groups) {
        if (group.files.empty()) continue;
        size_t oldestIdx = 0;
        int64_t oldestTime = group.files[0].lastModified;
        for (size_t i = 1; i < group.files.size(); ++i) {
            if (group.files[i].lastModified < oldestTime) {
                oldestTime = group.files[i].lastModified;
                oldestIdx = i;
            }
        }
        for (size_t i = 0; i < group.files.size(); ++i) {
            group.files[i].isSelectedForDeletion = (i != oldestIdx);
        }
    }
    syncTreeCheckStates(m_treeWidget, m_result);
    m_isUpdatingCheckState = false;
    updateSelectedStats();
}

void DuplicateFilesWidget::selectAllDuplicates() {
    m_isUpdatingCheckState = true;
    for (auto& group : m_result.groups) {
        for (size_t i = 0; i < group.files.size(); ++i) {
            group.files[i].isSelectedForDeletion = (i > 0);
        }
    }
    syncTreeCheckStates(m_treeWidget, m_result);
    m_isUpdatingCheckState = false;
    updateSelectedStats();
}

void DuplicateFilesWidget::deselectAll() {
    m_isUpdatingCheckState = true;
    for (auto& group : m_result.groups) {
        for (auto& f : group.files) {
            f.isSelectedForDeletion = false;
        }
    }
    syncTreeCheckStates(m_treeWidget, m_result);
    m_isUpdatingCheckState = false;
    updateSelectedStats();
}

void DuplicateFilesWidget::deleteSelectedToTrash() {
    int selectedCount = 0;
    int64_t selectedBytes = 0;
    for (const auto& group : m_result.groups) {
        for (const auto& file : group.files) {
            if (file.isSelectedForDeletion) {
                selectedCount++;
                selectedBytes += file.size;
            }
        }
    }

    if (selectedCount == 0) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("Confirm Recycle Bin Deletion"),
        QStringLiteral("Are you sure you want to move %1 duplicate file(s) (%2) to the Recycle Bin?\n\nFiles can be restored from the Recycle Bin if needed.")
            .arg(selectedCount)
            .arg(DiskNode::formatSize(static_cast<uint64_t>(selectedBytes))),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    int successCount = 0;
    int failCount = 0;

    for (auto& group : m_result.groups) {
        auto it = group.files.begin();
        while (it != group.files.end()) {
            if (it->isSelectedForDeletion) {
                if (QFile::moveToTrash(it->path)) {
                    successCount++;
                    it = group.files.erase(it);
                } else {
                    failCount++;
                    it->isSelectedForDeletion = false;
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    // Filter out groups with fewer than 2 files remaining
    auto groupIt = m_result.groups.begin();
    while (groupIt != m_result.groups.end()) {
        if (groupIt->files.size() < 2) {
            groupIt = m_result.groups.erase(groupIt);
        } else {
            ++groupIt;
        }
    }

    // Recompute summary metrics
    m_result.totalGroups = static_cast<int64_t>(m_result.groups.size());
    m_result.totalDuplicateFiles = 0;
    m_result.totalWastedBytes = 0;
    for (const auto& group : m_result.groups) {
        m_result.totalDuplicateFiles += group.files.size();
        m_result.totalWastedBytes += group.wastedBytes();
    }

    updateKpiDisplay();
    populateTree();
    updateSelectedStats();

    if (failCount > 0) {
        QMessageBox::warning(
            this,
            QStringLiteral("Partial Deletion"),
            QStringLiteral("Moved %1 file(s) to Recycle Bin, but failed to move %2 file(s) (permission denied or locked by another process).")
                .arg(successCount)
                .arg(failCount)
        );
    }

    m_statusLabel->setText(QStringLiteral("Recycle Bin cleanup complete: %1 file(s) moved to trash.")
        .arg(successCount));
}

void DuplicateFilesWidget::showContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) return;

    // Child file item
    if (item->parent() != nullptr) {
        QString filePath = item->data(0, Qt::UserRole).toString();
        if (filePath.isEmpty()) return;

        QMenu menu(this);
        menu.setStyleSheet(QStringLiteral(
            "QMenu { background-color: #161B22; color: #C9D1D9; border: 1px solid #30363D; padding: 4px; }"
            "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
            "QMenu::item:selected { background-color: #1F6FEB; color: #FFFFFF; }"
            "QMenu::separator { height: 1px; background-color: #30363D; margin: 4px 8px; }"
        ));

        QAction* openAct = menu.addAction(QStringLiteral("Open File"));
        QAction* explorerAct = menu.addAction(QStringLiteral("Show in Explorer"));
        QAction* copyPathAct = menu.addAction(QStringLiteral("Copy File Path"));
        menu.addSeparator();
        QAction* trashAct = menu.addAction(QStringLiteral("Move This File to Recycle Bin"));

        QAction* selected = menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
        if (selected == openAct) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        } else if (selected == explorerAct) {
            QString nativePath = QDir::toNativeSeparators(filePath);
            QProcess::startDetached(QStringLiteral("explorer.exe"), {QStringLiteral("/select,"), nativePath});
        } else if (selected == copyPathAct) {
            QGuiApplication::clipboard()->setText(filePath);
        } else if (selected == trashAct) {
            auto reply = QMessageBox::question(
                this,
                QStringLiteral("Confirm Recycle Bin Deletion"),
                QStringLiteral("Move \"%1\" to the Recycle Bin?").arg(QFileInfo(filePath).fileName()),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
            );
            if (reply == QMessageBox::Yes) {
                if (QFile::moveToTrash(filePath)) {
                    int groupIdx = item->data(1, Qt::UserRole).toInt();
                    int fileIdx = item->data(2, Qt::UserRole).toInt();
                    if (groupIdx >= 0 && groupIdx < static_cast<int>(m_result.groups.size())) {
                        auto& grp = m_result.groups[groupIdx];
                        if (fileIdx >= 0 && fileIdx < static_cast<int>(grp.files.size())) {
                            grp.files.erase(grp.files.begin() + fileIdx);
                        }
                    }
                    auto git = m_result.groups.begin();
                    while (git != m_result.groups.end()) {
                        if (git->files.size() < 2) {
                            git = m_result.groups.erase(git);
                        } else {
                            ++git;
                        }
                    }
                    m_result.totalGroups = static_cast<int64_t>(m_result.groups.size());
                    m_result.totalDuplicateFiles = 0;
                    m_result.totalWastedBytes = 0;
                    for (const auto& g : m_result.groups) {
                        m_result.totalDuplicateFiles += g.files.size();
                        m_result.totalWastedBytes += g.wastedBytes();
                    }
                    updateKpiDisplay();
                    populateTree();
                    updateSelectedStats();
                } else {
                    QMessageBox::warning(this, QStringLiteral("Error"), QStringLiteral("Failed to move file to Recycle Bin."));
                }
            }
        }
    }
}

