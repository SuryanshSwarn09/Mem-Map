#include "ThemeManager.h"

ThemeManager& ThemeManager::instance() {
    static ThemeManager s_instance;
    return s_instance;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
    updateTokens();
}

void ThemeManager::setTheme(ThemePreset preset) {
    if (m_preset != preset) {
        m_preset = preset;
        updateTokens();
        emit themeChanged(m_preset);
    }
}

void ThemeManager::updateTokens() {
    switch (m_preset) {
    case ThemePreset::ObsidianOLED:
        m_tokens.name = QStringLiteral("Obsidian OLED");
        m_tokens.surfaceDeep = QColor(QStringLiteral("#000000"));
        m_tokens.surfaceBase = QColor(QStringLiteral("#050505"));
        m_tokens.surfaceCard = QColor(QStringLiteral("#0D0D0D"));
        m_tokens.surfaceOverlay = QColor(QStringLiteral("#171717"));
        m_tokens.surfaceHover = QColor(QStringLiteral("#1F1F1F"));
        m_tokens.surfaceActive = QColor(QStringLiteral("#292929"));
        m_tokens.borderSubtle = QColor(QStringLiteral("#1C1C1C"));
        m_tokens.borderMuted = QColor(QStringLiteral("#2E2E2E"));
        m_tokens.borderFocus = QColor(QStringLiteral("#00FFCC"));
        m_tokens.textPrimary = QColor(QStringLiteral("#FFFFFF"));
        m_tokens.textSecondary = QColor(QStringLiteral("#D4D4D4"));
        m_tokens.textMuted = QColor(QStringLiteral("#737373"));
        m_tokens.accentPrimary = QColor(QStringLiteral("#0070F3"));
        m_tokens.accentCyan = QColor(QStringLiteral("#00E5FF"));
        m_tokens.accentGreen = QColor(QStringLiteral("#00E676"));
        m_tokens.accentRed = QColor(QStringLiteral("#FF1744"));
        m_tokens.accentAmber = QColor(QStringLiteral("#FFD600"));
        m_tokens.accentPurple = QColor(QStringLiteral("#D500F9"));
        break;

    case ThemePreset::NordicFrost:
        m_tokens.name = QStringLiteral("Nordic Frost");
        m_tokens.surfaceDeep = QColor(QStringLiteral("#0B0F19"));
        m_tokens.surfaceBase = QColor(QStringLiteral("#0F172A"));
        m_tokens.surfaceCard = QColor(QStringLiteral("#1E293B"));
        m_tokens.surfaceOverlay = QColor(QStringLiteral("#334155"));
        m_tokens.surfaceHover = QColor(QStringLiteral("#475569"));
        m_tokens.surfaceActive = QColor(QStringLiteral("#2563EB"));
        m_tokens.borderSubtle = QColor(QStringLiteral("#1E293B"));
        m_tokens.borderMuted = QColor(QStringLiteral("#334155"));
        m_tokens.borderFocus = QColor(QStringLiteral("#38BDF8"));
        m_tokens.textPrimary = QColor(QStringLiteral("#F8FAFC"));
        m_tokens.textSecondary = QColor(QStringLiteral("#E2E8F0"));
        m_tokens.textMuted = QColor(QStringLiteral("#94A3B8"));
        m_tokens.accentPrimary = QColor(QStringLiteral("#2563EB"));
        m_tokens.accentCyan = QColor(QStringLiteral("#38BDF8"));
        m_tokens.accentGreen = QColor(QStringLiteral("#10B981"));
        m_tokens.accentRed = QColor(QStringLiteral("#F43F5E"));
        m_tokens.accentAmber = QColor(QStringLiteral("#F59E0B"));
        m_tokens.accentPurple = QColor(QStringLiteral("#A855F7"));
        break;

    case ThemePreset::MidnightSlate:
    default:
        m_tokens.name = QStringLiteral("Midnight Slate");
        m_tokens.surfaceDeep = QColor(QStringLiteral("#090D12"));
        m_tokens.surfaceBase = QColor(QStringLiteral("#0E131A"));
        m_tokens.surfaceCard = QColor(QStringLiteral("#161B22"));
        m_tokens.surfaceOverlay = QColor(QStringLiteral("#1C2432"));
        m_tokens.surfaceHover = QColor(QStringLiteral("#21262D"));
        m_tokens.surfaceActive = QColor(QStringLiteral("#263346"));
        m_tokens.borderSubtle = QColor(QStringLiteral("#1E2734"));
        m_tokens.borderMuted = QColor(QStringLiteral("#30363D"));
        m_tokens.borderFocus = QColor(QStringLiteral("#58A6FF"));
        m_tokens.textPrimary = QColor(QStringLiteral("#F0F6FC"));
        m_tokens.textSecondary = QColor(QStringLiteral("#C9D1D9"));
        m_tokens.textMuted = QColor(QStringLiteral("#8B949E"));
        m_tokens.accentPrimary = QColor(QStringLiteral("#1F6FEB"));
        m_tokens.accentCyan = QColor(QStringLiteral("#58A6FF"));
        m_tokens.accentGreen = QColor(QStringLiteral("#238636"));
        m_tokens.accentRed = QColor(QStringLiteral("#F85149"));
        m_tokens.accentAmber = QColor(QStringLiteral("#D29922"));
        m_tokens.accentPurple = QColor(QStringLiteral("#8957E5"));
        break;
    }
}

QString ThemeManager::generateApplicationStyleSheet() const {
    const QString deep = m_tokens.surfaceDeep.name();
    const QString base = m_tokens.surfaceBase.name();
    const QString card = m_tokens.surfaceCard.name();
    const QString overlay = m_tokens.surfaceOverlay.name();
    const QString hover = m_tokens.surfaceHover.name();
    const QString active = m_tokens.surfaceActive.name();

    const QString bSubtle = m_tokens.borderSubtle.name();
    const QString bMuted = m_tokens.borderMuted.name();
    const QString bFocus = m_tokens.borderFocus.name();

    const QString tPrimary = m_tokens.textPrimary.name();
    const QString tSecondary = m_tokens.textSecondary.name();
    const QString tMuted = m_tokens.textMuted.name();

    const QString accPrimary = m_tokens.accentPrimary.name();
    const QString accCyan = m_tokens.accentCyan.name();

    return QStringLiteral(
        "QMainWindow { background-color: %1; }"
        "QWidget { color: %2; font-family: 'Segoe UI', -apple-system, sans-serif; font-size: 13px; }"

        // Controls
        "QComboBox, QLineEdit { "
        "   background-color: %3; border: 1px solid %4; border-radius: 6px; padding: 6px 12px; color: %2; "
        "}"
        "QComboBox:hover, QLineEdit:hover { background-color: %5; border-color: %6; }"
        "QComboBox:focus, QLineEdit:focus { border: 1px solid %7; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView { "
        "   background-color: %3; border: 1px solid %4; border-radius: 6px; selection-background-color: %8; color: %2; padding: 4px; "
        "}"

        // Buttons
        "QPushButton { "
        "   background-color: %5; border: 1px solid %4; border-radius: 6px; padding: 6px 14px; color: %2; font-weight: 500; "
        "}"
        "QPushButton:hover { background-color: %8; border-color: %6; color: %9; }"
        "QPushButton:pressed { background-color: %10; }"
        "QPushButton:disabled { background-color: %1; color: %11; border-color: %12; }"

        // Visualizer Toggle Buttons
        "QPushButton#visToggle { "
        "   background-color: %3; border: 1px solid %4; border-radius: 6px; padding: 5px 14px; color: %11; font-weight: bold; font-size: 12px; "
        "}"
        "QPushButton#visToggle:checked { "
        "   background-color: %8; border-color: %7; color: #FFFFFF; "
        "}"
        "QPushButton#visToggle:hover:!checked { background-color: %5; color: %2; }"

        // Pill-Capsule TabBar Styling
        "QTabWidget::pane { "
        "   border: 1px solid %4; background-color: %3; border-radius: 8px; top: -1px; "
        "}"
        "QTabBar::tab { "
        "   background-color: %1; color: %11; padding: 8px 18px; margin-right: 4px; "
        "   border-top-left-radius: 6px; border-top-right-radius: 6px; border: 1px solid %12; border-bottom: none; "
        "   font-weight: 500; "
        "}"
        "QTabBar::tab:selected { "
        "   background-color: %3; color: %7; font-weight: bold; border-color: %4; border-bottom: 1px solid %3; "
        "}"
        "QTabBar::tab:hover:!selected { background-color: %5; color: %9; }"

        // Trees & Tables
        "QTreeView, QTableWidget { "
        "   background-color: %3; border: none; gridline-color: %12; alternate-background-color: %1; "
        "   selection-background-color: %8; selection-color: #FFFFFF; outline: none; "
        "}"
        "QTreeView::item, QTableWidget::item { padding: 5px; border: none; border-radius: 3px; }"
        "QTreeView::item:hover, QTableWidget::item:hover { background-color: %5; }"
        "QTreeView::item:selected, QTableWidget::item:selected { background-color: %8; color: #FFFFFF; }"
        "QHeaderView::section { "
        "   background-color: %1; color: %11; padding: 7px 10px; border: none; "
        "   border-bottom: 1px solid %4; border-right: 1px solid %12; font-weight: bold; font-size: 11px; text-transform: uppercase; "
        "}"

        // Progress Bar
        "QProgressBar { background-color: %5; border: 1px solid %4; border-radius: 4px; text-align: center; font-size: 11px; }"
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %10, stop:1 %7); border-radius: 3px; "
        "}"

        // Splitter
        "QSplitter::handle { background-color: %12; width: 5px; }"
        "QSplitter::handle:hover { background-color: %7; }"

        // ScrollBars
        "QScrollBar:vertical { background-color: %1; width: 10px; margin: 0; }"
        "QScrollBar::handle:vertical { background-color: %4; min-height: 24px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background-color: %7; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background-color: %1; height: 10px; margin: 0; }"
        "QScrollBar::handle:horizontal { background-color: %4; min-width: 24px; border-radius: 5px; }"
        "QScrollBar::handle:horizontal:hover { background-color: %7; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

        // Context Menus
        "QMenu { "
        "   background-color: %3; border: 1px solid %4; border-radius: 8px; color: %2; padding: 6px; "
        "}"
        "QMenu::item { padding: 7px 24px 7px 14px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: %8; color: #FFFFFF; }"
        "QMenu::separator { height: 1px; background-color: %12; margin: 4px 6px; }"

        // Tooltips
        "QToolTip { "
        "   background-color: %13; border: 1px solid %7; color: #FFFFFF; padding: 7px 12px; border-radius: 6px; font-size: 12px; "
        "}"
    ).arg(deep)          // 1
     .arg(tSecondary)    // 2
     .arg(card)          // 3
     .arg(bMuted)         // 4
     .arg(hover)         // 5
     .arg(tMuted)        // 6
     .arg(bFocus)        // 7
     .arg(active)        // 8
     .arg(tPrimary)      // 9
     .arg(accPrimary)    // 10
     .arg(tMuted)        // 11
     .arg(bSubtle)       // 12
     .arg(overlay);      // 13
}
