#include "BreadcrumbWidget.h"
#include "ThemeManager.h"
#include <QLabel>
#include <vector>

BreadcrumbWidget::BreadcrumbWidget(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(6, 4, 6, 4);
    m_layout->setSpacing(6);

    m_btnUp = new QPushButton(QStringLiteral("▲ Up"), this);
    m_btnUp->setToolTip(QStringLiteral("Navigate to parent folder (Backspace / Alt+Up)"));
    m_btnUp->setEnabled(false);

    m_btnRoot = new QPushButton(QStringLiteral("🏠 Root"), this);
    m_btnRoot->setToolTip(QStringLiteral("Reset to scan root"));
    m_btnRoot->setEnabled(false);

    m_layout->addWidget(m_btnUp);
    m_layout->addWidget(m_btnRoot);

    connect(m_btnUp, &QPushButton::clicked, this, &BreadcrumbWidget::upRequested);
    connect(m_btnRoot, &QPushButton::clicked, this, &BreadcrumbWidget::resetRequested);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](ThemePreset) {
        setRootAndCurrent(m_scanRoot, m_currentFolder);
    });

    m_layout->addStretch();
    setRootAndCurrent(nullptr, nullptr);
}

void BreadcrumbWidget::setRootAndCurrent(DiskNode* scanRoot, DiskNode* currentFolder) {
    m_scanRoot = scanRoot;
    m_currentFolder = currentFolder;

    const auto& tokens = ThemeManager::instance().tokens();
    const QString base = tokens.surfaceBase.name();
    const QString card = tokens.surfaceCard.name();
    const QString hover = tokens.surfaceHover.name();
    const QString bMuted = tokens.borderMuted.name();
    const QString bSubtle = tokens.borderSubtle.name();
    const QString tMuted = tokens.textMuted.name();
    const QString tSec = tokens.textSecondary.name();
    const QString accPrim = tokens.accentPrimary.name();
    const QString accCyan = tokens.accentCyan.name();

    setStyleSheet(QStringLiteral(
        "BreadcrumbWidget { background-color: %1; border: 1px solid %2; border-radius: 8px; }"
    ).arg(base, bSubtle));

    QString navPillStyle = QStringLiteral(
        "QPushButton { background-color: %1; border: 1px solid %2; border-radius: 10px; padding: 3px 10px; color: %3; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background-color: %4; color: #FFFFFF; border-color: %5; }"
        "QPushButton:disabled { background-color: transparent; color: %6; border-color: %7; }"
    ).arg(card, bMuted, tSec, hover, accCyan, tMuted, bSubtle);

    QString activePillStyle = QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %1, stop:1 %2); border: 1px solid %2; border-radius: 10px; padding: 3px 12px; color: #FFFFFF; font-weight: 600; font-size: 11px; }"
        "QPushButton:hover { border-color: #FFFFFF; }"
    ).arg(accPrim, accCyan);

    m_btnUp->setStyleSheet(navPillStyle);
    m_btnRoot->setStyleSheet(navPillStyle);

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

    // Add path buttons in forward order (root -> ... -> current)
    for (int i = static_cast<int>(pathNodes.size()) - 1; i >= 0; --i) {
        DiskNode* node = pathNodes[i];

        if (i < static_cast<int>(pathNodes.size()) - 1) {
            QLabel* arrow = new QLabel(QStringLiteral("›"), this);
            arrow->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; font-weight: bold; padding: 0 2px;").arg(tMuted));
            m_layout->addWidget(arrow);
        }

        QPushButton* btn = new QPushButton(node->name().isEmpty() ? node->fullPath() : node->name(), this);
        bool isCurrent = (node == currentFolder);

        btn->setStyleSheet(isCurrent ? activePillStyle : navPillStyle);

        connect(btn, &QPushButton::clicked, this, [this, node]() {
            emit folderSelected(node);
        });

        m_layout->addWidget(btn);
    }

    m_layout->addStretch();
}
