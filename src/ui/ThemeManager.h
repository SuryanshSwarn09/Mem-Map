#pragma once

#include <QString>
#include <QColor>
#include <QObject>

enum class ThemePreset {
    MidnightSlate, // Modern Linear/GitHub Dark Slate (Default)
    ObsidianOLED,  // Pure black #000000 with high-contrast neon accents
    NordicFrost    // Deep Arctic Indigo Slate with frost-blue accents
};

struct ThemeTokens {
    QString name;
    
    // Surfaces
    QColor surfaceDeep;     // Window canvas / background
    QColor surfaceBase;     // Central content panels
    QColor surfaceCard;     // Cards & containers
    QColor surfaceOverlay;  // Menus, tooltips
    QColor surfaceHover;    // Hover states
    QColor surfaceActive;   // Selected / active item

    // Borders
    QColor borderSubtle;    // Dividers
    QColor borderMuted;     // Standard element borders
    QColor borderFocus;     // High-contrast active borders

    // Typography
    QColor textPrimary;     // Main headings & titles
    QColor textSecondary;   // Body labels
    QColor textMuted;       // Subtitles, metadata

    // Accents
    QColor accentPrimary;   // Brand Blue
    QColor accentCyan;      // Active highlights
    QColor accentGreen;     // Success & safe cleanup
    QColor accentRed;       // Danger / wasted space
    QColor accentAmber;     // Warnings / age
    QColor accentPurple;    // Special categories
};

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();

    [[nodiscard]] ThemePreset currentPreset() const { return m_preset; }
    [[nodiscard]] const ThemeTokens& tokens() const { return m_tokens; }

    void setTheme(ThemePreset preset);
    [[nodiscard]] QString generateApplicationStyleSheet() const;

signals:
    void themeChanged(ThemePreset preset);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    void updateTokens();

    ThemePreset m_preset{ThemePreset::MidnightSlate};
    ThemeTokens m_tokens;
};
