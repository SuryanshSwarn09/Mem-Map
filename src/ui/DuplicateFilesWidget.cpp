#include "DuplicateFilesWidget.h"
#include "core/DiskNode.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

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

// Scaffolds to be populated in upcoming micro-commits:
void DuplicateFilesWidget::createActionToolbar() {}
void DuplicateFilesWidget::createResultsTree() {}
void DuplicateFilesWidget::populateTree() {}
void DuplicateFilesWidget::updateSelectedStats() {}
void DuplicateFilesWidget::startScan() {}
void DuplicateFilesWidget::cancelScan() {}
void DuplicateFilesWidget::selectKeepNewest() {}
void DuplicateFilesWidget::selectKeepOldest() {}
void DuplicateFilesWidget::selectAllDuplicates() {}
void DuplicateFilesWidget::deselectAll() {}
void DuplicateFilesWidget::deleteSelectedToTrash() {}
void DuplicateFilesWidget::filterFiles(const QString& query) { Q_UNUSED(query); }
void DuplicateFilesWidget::onScanStarted() {}
void DuplicateFilesWidget::onScanFinished(const DuplicateScanResult& result) { Q_UNUSED(result); }
void DuplicateFilesWidget::onScanCancelled() {}
void DuplicateFilesWidget::onItemChanged(QTreeWidgetItem* item, int column) { Q_UNUSED(item); Q_UNUSED(column); }
void DuplicateFilesWidget::showContextMenu(const QPoint& pos) { Q_UNUSED(pos); }
