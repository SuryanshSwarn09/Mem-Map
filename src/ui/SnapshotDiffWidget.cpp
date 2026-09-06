#include "SnapshotDiffWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QDir>
#include <QFileIconProvider>
#include <cmath>

static QString formatDelta(int64_t bytes) {
    if (bytes > 0) {
        return QStringLiteral("+") + DiskNode::formatSize(static_cast<uint64_t>(bytes));
    } else if (bytes < 0) {
        return QStringLiteral("-") + DiskNode::formatSize(static_cast<uint64_t>(-bytes));
    }
    return QStringLiteral("0 B");
}

SnapshotDiffWidget::SnapshotDiffWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

void SnapshotDiffWidget::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(10);

    // 1. KPI Summary Banner
    QWidget* banner = new QWidget(this);
    banner->setStyleSheet(QStringLiteral("background-color: #161B22; border: 1px solid #30363D; border-radius: 8px;"));
    QHBoxLayout* bannerLayout = new QHBoxLayout(banner);
    bannerLayout->setContentsMargins(16, 12, 16, 12);
    bannerLayout->setSpacing(20);

    // Left card: Old vs New
    QVBoxLayout* scansLayout = new QVBoxLayout();
    m_lblOldScan = new QLabel(QStringLiteral("<b>Baseline Scan:</b> None loaded"), banner);
    m_lblOldScan->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 12px; border:none;"));
    m_lblNewScan = new QLabel(QStringLiteral("<b>Current Scan:</b> None loaded"), banner);
    m_lblNewScan->setStyleSheet(QStringLiteral("color: #C9D1D9; font-size: 12px; border:none;"));
    scansLayout->addWidget(m_lblOldScan);
    scansLayout->addWidget(m_lblNewScan);
    bannerLayout->addLayout(scansLayout, 2);

    // Center card: Net Delta
    QVBoxLayout* deltaLayout = new QVBoxLayout();
    QLabel* lblDeltaTitle = new QLabel(QStringLiteral("NET STORAGE CHANGE"), banner);
    lblDeltaTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; border:none;"));
    m_lblNetChange = new QLabel(QStringLiteral("0 B"), banner);
    m_lblNetChange->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 20px; font-weight: bold; border:none;"));
    deltaLayout->addWidget(lblDeltaTitle);
    deltaLayout->addWidget(m_lblNetChange);
    bannerLayout->addLayout(deltaLayout, 1);

    // Right card: Item breakdown counts
    QVBoxLayout* countsLayout = new QVBoxLayout();
    QLabel* lblBreakdownTitle = new QLabel(QStringLiteral("CHANGED ITEMS"), banner);
    lblBreakdownTitle->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 11px; font-weight: bold; border:none;"));
    m_lblCounts = new QLabel(QStringLiteral("Added: 0 | Deleted: 0 | Modified: 0"), banner);
    m_lblCounts->setStyleSheet(QStringLiteral("color: #C9D1D9; font-size: 12px; border:none;"));
    countsLayout->addWidget(lblBreakdownTitle);
    countsLayout->addWidget(m_lblCounts);
    bannerLayout->addLayout(countsLayout, 2);

    rootLayout->addWidget(banner);

    // 2. Filter Bar
    QHBoxLayout* filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(6);

    QLabel* lblFilter = new QLabel(QStringLiteral("Filter:"), this);
    lblFilter->setStyleSheet(QStringLiteral("font-weight: bold; color: #8B949E;"));
    filterLayout->addWidget(lblFilter);

    m_btnFilterAll = new QPushButton(QStringLiteral("All Changes"), this);
    m_btnFilterGrowth = new QPushButton(QStringLiteral("📈 Growth (+)"), this);
    m_btnFilterFreed = new QPushButton(QStringLiteral("📉 Freed Space (-)"), this);
    m_btnFilterAdded = new QPushButton(QStringLiteral("✨ Added"), this);
    m_btnFilterDeleted = new QPushButton(QStringLiteral("🗑 Deleted"), this);

    for (QPushButton* btn : {m_btnFilterAll, m_btnFilterGrowth, m_btnFilterFreed, m_btnFilterAdded, m_btnFilterDeleted}) {
        btn->setCheckable(true);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: #21262D; border: 1px solid #30363D; border-radius: 4px; padding: 4px 10px; color: #8B949E; font-weight: 600; font-size: 12px; }"
            "QPushButton:checked { background-color: #1F6FEB; border-color: #388BFD; color: #FFFFFF; }"
            "QPushButton:hover:!checked { background-color: #30363D; color: #C9D1D9; }"
        ));
    }
    m_btnFilterAll->setChecked(true);

    connect(m_btnFilterAll, &QPushButton::clicked, this, [this]() { onFilterChanged(DiffFilter::All); });
    connect(m_btnFilterGrowth, &QPushButton::clicked, this, [this]() { onFilterChanged(DiffFilter::GrowthOnly); });
    connect(m_btnFilterFreed, &QPushButton::clicked, this, [this]() { onFilterChanged(DiffFilter::FreedOnly); });
    connect(m_btnFilterAdded, &QPushButton::clicked, this, [this]() { onFilterChanged(DiffFilter::AddedOnly); });
    connect(m_btnFilterDeleted, &QPushButton::clicked, this, [this]() { onFilterChanged(DiffFilter::DeletedOnly); });

    filterLayout->addWidget(m_btnFilterAll);
    filterLayout->addWidget(m_btnFilterGrowth);
    filterLayout->addWidget(m_btnFilterFreed);
    filterLayout->addWidget(m_btnFilterAdded);
    filterLayout->addWidget(m_btnFilterDeleted);
    filterLayout->addSpacing(12);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("🔍 Filter by path or filename..."));
    m_searchEdit->setStyleSheet(QStringLiteral("background-color: #21262D; border: 1px solid #30363D; border-radius: 4px; padding: 4px 8px; color: #C9D1D9;"));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SnapshotDiffWidget::onSearchTextChanged);
    filterLayout->addWidget(m_searchEdit, 1);

    rootLayout->addLayout(filterLayout);

    // 3. Tree Widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(6);
    m_treeWidget->setHeaderLabels({
        QStringLiteral("Name"),
        QStringLiteral("Net Change"),
        QStringLiteral("Current Size"),
        QStringLiteral("Baseline Size"),
        QStringLiteral("Status"),
        QStringLiteral("Path")
    });

    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(5, QHeaderView::Interactive);
    m_treeWidget->header()->resizeSection(5, 200);

    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &SnapshotDiffWidget::onTreeContextMenu);

    rootLayout->addWidget(m_treeWidget, 1);
}

void SnapshotDiffWidget::setDiffData(std::unique_ptr<DiffNode> diffRoot, const DiffSummary& summary) {
    m_diffRoot = std::move(diffRoot);
    m_summary = summary;
    updateSummaryCards(m_summary);
    populateTree();
}

void SnapshotDiffWidget::clear() {
    m_diffRoot.reset();
    m_summary = DiffSummary{};
    m_lblOldScan->setText(QStringLiteral("<b>Baseline Scan:</b> None loaded"));
    m_lblNewScan->setText(QStringLiteral("<b>Current Scan:</b> None loaded"));
    m_lblNetChange->setText(QStringLiteral("0 B"));
    m_lblNetChange->setStyleSheet(QStringLiteral("color: #FFFFFF; font-size: 20px; font-weight: bold; border:none;"));
    m_lblCounts->setText(QStringLiteral("Added: 0 | Deleted: 0 | Modified: 0"));
    m_treeWidget->clear();
}

void SnapshotDiffWidget::updateSummaryCards(const DiffSummary& summary) {
    m_lblOldScan->setText(QStringLiteral("<b>Baseline:</b> %1 (%2)")
                          .arg(summary.oldPath, DiskNode::formatSize(summary.oldTotalBytes)));
    m_lblNewScan->setText(QStringLiteral("<b>Current:</b> %1 (%2)")
                          .arg(summary.newPath, DiskNode::formatSize(summary.newTotalBytes)));

    QString deltaStr = formatDelta(summary.deltaTotalBytes);
    m_lblNetChange->setText(deltaStr);
    if (summary.deltaTotalBytes > 0) {
        m_lblNetChange->setStyleSheet(QStringLiteral("color: #F85149; font-size: 20px; font-weight: bold; border:none;")); // Red growth
    } else if (summary.deltaTotalBytes < 0) {
        m_lblNetChange->setStyleSheet(QStringLiteral("color: #3FB950; font-size: 20px; font-weight: bold; border:none;")); // Green reduction
    } else {
        m_lblNetChange->setStyleSheet(QStringLiteral("color: #8B949E; font-size: 20px; font-weight: bold; border:none;"));
    }

    m_lblCounts->setText(QStringLiteral("<span style='color:#3FB950;'>+ Added: %1</span> &bull; <span style='color:#F85149;'>- Deleted: %2</span> &bull; <span style='color:#58A6FF;'>~ Modified: %3</span>")
                         .arg(summary.addedCount)
                         .arg(summary.deletedCount)
                         .arg(summary.modifiedCount));
}

void SnapshotDiffWidget::onFilterChanged(DiffFilter filter) {
    m_currentFilter = filter;
    m_btnFilterAll->setChecked(filter == DiffFilter::All);
    m_btnFilterGrowth->setChecked(filter == DiffFilter::GrowthOnly);
    m_btnFilterFreed->setChecked(filter == DiffFilter::FreedOnly);
    m_btnFilterAdded->setChecked(filter == DiffFilter::AddedOnly);
    m_btnFilterDeleted->setChecked(filter == DiffFilter::DeletedOnly);
    populateTree();
}

void SnapshotDiffWidget::onSearchTextChanged(const QString& /*text*/) {
    populateTree();
}

bool SnapshotDiffWidget::shouldIncludeNode(const DiffNode* node, const QString& searchLower) const {
    if (!node) return false;

    // Search query matching
    if (!searchLower.isEmpty()) {
        bool matches = node->name.toLower().contains(searchLower) || node->fullPath.toLower().contains(searchLower);
        if (!matches) {
            // Check if any child matches
            bool childMatches = false;
            for (const auto& c : node->children) {
                if (shouldIncludeNode(c.get(), searchLower)) {
                    childMatches = true;
                    break;
                }
            }
            if (!childMatches) return false;
        }
    }

    // Filter type matching
    switch (m_currentFilter) {
        case DiffFilter::All:
            return node->status != DiffStatus::Unchanged || !node->children.empty();
        case DiffFilter::GrowthOnly:
            return node->deltaBytes > 0;
        case DiffFilter::FreedOnly:
            return node->deltaBytes < 0;
        case DiffFilter::AddedOnly:
            return node->status == DiffStatus::Added || node->deltaBytes > 0;
        case DiffFilter::DeletedOnly:
            return node->status == DiffStatus::Deleted || node->deltaBytes < 0;
    }
    return true;
}

void SnapshotDiffWidget::addDiffItem(QTreeWidgetItem* parentItem, const DiffNode* node, const QString& searchLower) {
    if (!node || !shouldIncludeNode(node, searchLower)) return;

    QTreeWidgetItem* item = parentItem ? new QTreeWidgetItem(parentItem) : new QTreeWidgetItem(m_treeWidget);
    QFileIconProvider iconProvider;

    // Col 0: Name
    item->setText(0, node->name);
    item->setIcon(0, iconProvider.icon(node->isDir ? QFileIconProvider::Folder : QFileIconProvider::File));

    // Col 1: Net Change
    item->setText(1, formatDelta(node->deltaBytes));
    item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    if (node->deltaBytes > 0) {
        item->setForeground(1, QColor(248, 81, 73));   // Red / growth
    } else if (node->deltaBytes < 0) {
        item->setForeground(1, QColor(63, 185, 80));   // Green / freed
    } else {
        item->setForeground(1, QColor(139, 148, 158));
    }

    // Col 2: New Size
    item->setText(2, DiskNode::formatSize(node->newSize));
    item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);

    // Col 3: Old Size
    item->setText(3, DiskNode::formatSize(node->oldSize));
    item->setTextAlignment(3, Qt::AlignRight | Qt::AlignVCenter);

    // Col 4: Status
    item->setText(4, DiffNode::statusToString(node->status));
    switch (node->status) {
        case DiffStatus::Added:
            item->setForeground(4, QColor(63, 185, 80));  // Green
            break;
        case DiffStatus::Deleted:
            item->setForeground(4, QColor(248, 81, 73));  // Red
            break;
        case DiffStatus::Modified:
            item->setForeground(4, QColor(88, 166, 255)); // Blue
            break;
        case DiffStatus::Unchanged:
            item->setForeground(4, QColor(139, 148, 158));
            break;
    }

    // Col 5: Full Path
    item->setText(5, node->fullPath);
    item->setData(0, Qt::UserRole, node->fullPath);

    for (const auto& child : node->children) {
        addDiffItem(item, child.get(), searchLower);
    }
}

void SnapshotDiffWidget::populateTree() {
    m_treeWidget->clear();
    if (!m_diffRoot) return;

    QString search = m_searchEdit->text().trimmed().toLower();
    addDiffItem(nullptr, m_diffRoot.get(), search);

    if (m_treeWidget->topLevelItemCount() > 0) {
        m_treeWidget->topLevelItem(0)->setExpanded(true);
    }
}

void SnapshotDiffWidget::onTreeContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    if (!item) return;

    QString path = item->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return;

    QMenu menu(this);
    QAction* actOpen = menu.addAction(QStringLiteral("Open in File Explorer"));
    QAction* actCopy = menu.addAction(QStringLiteral("Copy Full Path"));

    QAction* selected = menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
    if (selected == actOpen) {
#ifdef _WIN32
        QString nativePath = QDir::toNativeSeparators(path);
        QString param = QStringLiteral("/select,\"%1\"").arg(nativePath);
        QProcess::startDetached(QStringLiteral("explorer.exe"), {param});
#endif
    } else if (selected == actCopy) {
        QGuiApplication::clipboard()->setText(path);
    }
}
