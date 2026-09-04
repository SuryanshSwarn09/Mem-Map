#include "ExtensionStatsWidget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <algorithm>

ExtensionStatsWidget::ExtensionStatsWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("Extension"),
        QStringLiteral("Category"),
        QStringLiteral("Size"),
        QStringLiteral("Share %"),
        QStringLiteral("Files")
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSortingEnabled(false);
    m_table->verticalHeader()->setVisible(false);

    layout->addWidget(m_table);
}

void ExtensionStatsWidget::clear() {
    m_table->setRowCount(0);
}

void ExtensionStatsWidget::collectStats(DiskNode* node, std::map<QString, ExtensionSummary>& stats) {
    if (!node) return;

    if (!node->isDirectory()) {
        QString ext = node->extension();
        auto& item = stats[ext];
        if (item.ext.isEmpty()) {
            item.ext = ext;
            item.category = DiskNode::getFileCategory(ext);
            item.color = DiskNode::getColorForExtension(ext);
        }
        item.totalBytes += node->size();
        item.fileCount += 1;
        return;
    }

    for (const auto& child : node->children()) {
        collectStats(child.get(), stats);
    }
}

void ExtensionStatsWidget::populateFromNode(DiskNode* rootNode) {
    clear();
    if (!rootNode || rootNode->size() == 0) return;

    std::map<QString, ExtensionSummary> statsMap;
    collectStats(rootNode, statsMap);

    std::vector<ExtensionSummary> list;
    list.reserve(statsMap.size());
    for (const auto& pair : statsMap) {
        list.push_back(pair.second);
    }

    // Sort descending by total size
    std::sort(list.begin(), list.end(), [](const ExtensionSummary& a, const ExtensionSummary& b) {
        return a.totalBytes > b.totalBytes;
    });

    m_table->setRowCount(static_cast<int>(list.size()));
    double totalBytes = static_cast<double>(rootNode->size());

    for (int r = 0; r < static_cast<int>(list.size()); ++r) {
        const auto& s = list[r];

        // Color badge icon
        QPixmap pix(14, 14);
        pix.fill(Qt::transparent);
        {
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(s.color);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(0, 0, 14, 14, 3, 3);
        }

        // Col 0: Extension
        QTableWidgetItem* itemExt = new QTableWidgetItem(s.ext == QStringLiteral("(none)") ? QStringLiteral("[No ext]") : ("." + s.ext));
        itemExt->setIcon(QIcon(pix));
        m_table->setItem(r, 0, itemExt);

        // Col 1: Category
        QTableWidgetItem* itemCat = new QTableWidgetItem(s.category);
        m_table->setItem(r, 1, itemCat);

        // Col 2: Size
        QTableWidgetItem* itemSize = new QTableWidgetItem(DiskNode::formatSize(s.totalBytes));
        itemSize->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 2, itemSize);

        // Col 3: Share %
        double share = (totalBytes > 0) ? (static_cast<double>(s.totalBytes) / totalBytes * 100.0) : 0.0;
        QTableWidgetItem* itemShare = new QTableWidgetItem(QString::asprintf("%.1f%%", share));
        itemShare->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 3, itemShare);

        // Col 4: Files count
        QTableWidgetItem* itemFiles = new QTableWidgetItem(QString::number(s.fileCount));
        itemFiles->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 4, itemFiles);
    }
}
