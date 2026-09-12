#include "TopFilesWidget.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QProcess>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <algorithm>

TopFilesWidget::TopFilesWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("#"),
        QStringLiteral("File Name"),
        QStringLiteral("Size"),
        QStringLiteral("Type"),
        QStringLiteral("Folder Path")
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->resizeSection(1, 220);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);

    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int /*col*/) {
        if (row >= 0 && row < static_cast<int>(m_topNodes.size())) {
            emit fileSelected(m_topNodes[row]);
        }
    });

    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        int row = m_table->rowAt(pos.y());
        if (row < 0 || row >= static_cast<int>(m_topNodes.size())) return;

        DiskNode* node = m_topNodes[row];
        QMenu menu(this);
        QAction* actOpen = menu.addAction(QStringLiteral("Open in File Explorer"));
        QAction* actCopy = menu.addAction(QStringLiteral("Copy Full Path"));
        QAction* actShow = menu.addAction(QStringLiteral("Locate in Tree View"));

        QAction* selected = menu.exec(m_table->viewport()->mapToGlobal(pos));
        if (selected == actOpen) {
#ifdef _WIN32
            QString path = QDir::toNativeSeparators(node->fullPath());
            QString param = QStringLiteral("/select,\"%1\"").arg(path);
            QProcess::startDetached(QStringLiteral("explorer.exe"), {param});
#endif
        } else if (selected == actCopy) {
            QGuiApplication::clipboard()->setText(node->fullPath());
        } else if (selected == actShow) {
            emit fileSelected(node);
        }
    });
}

void TopFilesWidget::clear() {
    m_table->setRowCount(0);
    m_topNodes.clear();
}

void TopFilesWidget::collectFiles(DiskNode* node, std::vector<DiskNode*>& files) {
    if (!node) return;

    if (!node->isDirectory()) {
        if (node->size() > 0) {
            files.push_back(node);
        }
        return;
    }

    for (const auto& child : node->children()) {
        collectFiles(child.get(), files);
    }
}

void TopFilesWidget::populateFromNode(DiskNode* rootNode, int maxCount) {
    clear();
    if (!rootNode) return;

    std::vector<DiskNode*> allFiles;
    collectFiles(rootNode, allFiles);

    if (allFiles.empty()) return;

    size_t count = std::min(static_cast<size_t>(maxCount), allFiles.size());
    std::partial_sort(allFiles.begin(), allFiles.begin() + count, allFiles.end(),
                      [](DiskNode* a, DiskNode* b) {
                          return a->size() > b->size();
                      });

    m_topNodes.assign(allFiles.begin(), allFiles.begin() + count);
    m_table->setRowCount(static_cast<int>(m_topNodes.size()));

    const auto& tokens = ThemeManager::instance().tokens();

    for (int r = 0; r < static_cast<int>(m_topNodes.size()); ++r) {
        DiskNode* node = m_topNodes[r];

        // Col 0: Rank with medals for top 3
        QString rankStr = QString::number(r + 1);
        if (r == 0) rankStr = QStringLiteral("🥇 1");
        else if (r == 1) rankStr = QStringLiteral("🥈 2");
        else if (r == 2) rankStr = QStringLiteral("🥉 3");

        QTableWidgetItem* itemRank = new QTableWidgetItem(rankStr);
        itemRank->setTextAlignment(Qt::AlignCenter);
        if (r < 3) {
            QFont f = itemRank->font();
            f.setBold(true);
            itemRank->setFont(f);
        }
        m_table->setItem(r, 0, itemRank);

        // Col 1: File Name
        QTableWidgetItem* itemName = new QTableWidgetItem(node->name());
        m_table->setItem(r, 1, itemName);

        // Col 2: Size
        QTableWidgetItem* itemSize = new QTableWidgetItem(DiskNode::formatSize(node->size()));
        itemSize->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(r, 2, itemSize);

        // Col 3: Type with category color badge
        QColor extColor = DiskNode::getColorForExtension(node->extension());
        QPixmap typePix(16, 16);
        typePix.fill(Qt::transparent);
        {
            QPainter p(&typePix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(extColor);
            p.setPen(QPen(extColor.lighter(135), 1.0));
            p.drawRoundedRect(1, 1, 14, 14, 4, 4);
        }

        QTableWidgetItem* itemType = new QTableWidgetItem(node->extension().toUpper());
        itemType->setIcon(QIcon(typePix));
        itemType->setForeground(extColor.lighter(120));
        m_table->setItem(r, 3, itemType);

        // Col 4: Path
        QTableWidgetItem* itemPath = new QTableWidgetItem(node->parent() ? node->parent()->fullPath() : node->fullPath());
        itemPath->setForeground(tokens.textMuted);
        m_table->setItem(r, 4, itemPath);
    }
}
