#include "BreadcrumbWidget.h"
#include <QLabel>
#include <vector>

BreadcrumbWidget::BreadcrumbWidget(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 2, 4, 2);
    m_layout->setSpacing(4);

    m_btnUp = new QPushButton(QStringLiteral("▲ Up"), this);
    m_btnUp->setToolTip(QStringLiteral("Navigate to parent folder"));
    m_btnUp->setEnabled(false);
    m_btnUp->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #2D333B; border: 1px solid #444C56; border-radius: 4px; padding: 4px 8px; color: #ADBAC7; }"
        "QPushButton:hover { background-color: #373E47; color: #FFFFFF; border-color: #539BF5; }"
        "QPushButton:disabled { background-color: #22272E; color: #545D68; border-color: #373E47; }"
    ));

    m_btnRoot = new QPushButton(QStringLiteral("🏠 Root"), this);
    m_btnRoot->setToolTip(QStringLiteral("Reset to scan root"));
    m_btnRoot->setEnabled(false);
    m_btnRoot->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #2D333B; border: 1px solid #444C56; border-radius: 4px; padding: 4px 8px; color: #ADBAC7; }"
        "QPushButton:hover { background-color: #373E47; color: #FFFFFF; border-color: #539BF5; }"
        "QPushButton:disabled { background-color: #22272E; color: #545D68; border-color: #373E47; }"
    ));

    m_layout->addWidget(m_btnUp);
    m_layout->addWidget(m_btnRoot);

    connect(m_btnUp, &QPushButton::clicked, this, &BreadcrumbWidget::upRequested);
    connect(m_btnRoot, &QPushButton::clicked, this, &BreadcrumbWidget::resetRequested);

    m_layout->addStretch();
}

void BreadcrumbWidget::setRootAndCurrent(DiskNode* scanRoot, DiskNode* currentFolder) {
    m_scanRoot = scanRoot;
    m_currentFolder = currentFolder;

    // Clear old buttons (except Up and Root)
    QLayoutItem* item;
    while (m_layout->count() > 2) {
        item = m_layout->takeAt(2);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    if (!scanRoot || !currentFolder) {
        m_btnUp->setEnabled(false);
        m_btnRoot->setEnabled(false);
        m_layout->addStretch();
        return;
    }

    m_btnUp->setEnabled(currentFolder != scanRoot && currentFolder->parent() != nullptr);
    m_btnRoot->setEnabled(currentFolder != scanRoot);

    // Build path from currentFolder up to scanRoot
    std::vector<DiskNode*> pathNodes;
    DiskNode* curr = currentFolder;
    while (curr) {
        pathNodes.push_back(curr);
        if (curr == scanRoot) break;
        curr = curr->parent();
    }

    // Now add buttons in forward order (root -> ... -> current)
    for (int i = static_cast<int>(pathNodes.size()) - 1; i >= 0; --i) {
        DiskNode* node = pathNodes[i];

        if (i < static_cast<int>(pathNodes.size()) - 1) {
            QLabel* arrow = new QLabel(QStringLiteral("›"), this);
            arrow->setStyleSheet(QStringLiteral("color: #768390; font-size: 14px; font-weight: bold;"));
            m_layout->addWidget(arrow);
        }

        QPushButton* btn = new QPushButton(node->name().isEmpty() ? node->fullPath() : node->name(), this);
        bool isCurrent = (node == currentFolder);

        if (isCurrent) {
            btn->setStyleSheet(QStringLiteral(
                "QPushButton { background-color: #1F6FEB; border: 1px solid #388BFD; border-radius: 4px; padding: 4px 8px; color: #FFFFFF; font-weight: bold; }"
            ));
        } else {
            btn->setStyleSheet(QStringLiteral(
                "QPushButton { background-color: #2D333B; border: 1px solid #444C56; border-radius: 4px; padding: 4px 8px; color: #ADBAC7; }"
                "QPushButton:hover { background-color: #373E47; color: #FFFFFF; border-color: #539BF5; }"
            ));
        }

        connect(btn, &QPushButton::clicked, this, [this, node]() {
            emit folderSelected(node);
        });

        m_layout->addWidget(btn);
    }

    m_layout->addStretch();
}
