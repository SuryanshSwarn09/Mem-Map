#include "SizeBarDelegate.h"
#include "ThemeManager.h"
#include <QPainter>
#include <QPainterPath>

SizeBarDelegate::SizeBarDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

void SizeBarDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    double percent = index.data(Qt::UserRole).toDouble(); // 0.0 to 100.0

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const auto& tokens = ThemeManager::instance().tokens();

    // Selected or hovered background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, tokens.surfaceActive);
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect, tokens.surfaceHover);
    }

    // Bar dimensions
    int padX = 6;
    int padY = 5;
    QRectF barRect = option.rect.adjusted(padX, padY, -padX, -padY);

    if (barRect.width() > 10 && barRect.height() > 4) {
        qreal pillRadius = barRect.height() / 2.0;

        // Track background
        QPainterPath trackPath;
        trackPath.addRoundedRect(barRect, pillRadius, pillRadius);
        painter->fillPath(trackPath, tokens.surfaceCard);

        // Subtle track border
        painter->setPen(QPen(tokens.borderSubtle, 1.0));
        painter->drawPath(trackPath);

        // Fill bar
        qreal fillRatio = qBound(0.0, percent, 100.0) / 100.0;
        qreal fillWidth = barRect.width() * fillRatio;
        if (fillWidth > 2.0) {
            QRectF filledRect(barRect.x(), barRect.y(), fillWidth, barRect.height());
            QPainterPath fillPath;
            fillPath.addRoundedRect(filledRect, pillRadius, pillRadius);

            QLinearGradient grad(filledRect.topLeft(), filledRect.topRight());
            if (percent > 70.0) {
                grad.setColorAt(0.0, tokens.accentRed);
                grad.setColorAt(1.0, QColor(255, 107, 107));
            } else if (percent > 35.0) {
                grad.setColorAt(0.0, tokens.accentAmber);
                grad.setColorAt(1.0, QColor(255, 217, 61));
            } else {
                grad.setColorAt(0.0, tokens.accentPrimary);
                grad.setColorAt(1.0, tokens.accentCyan);
            }

            painter->fillPath(fillPath, grad);
        }

        // Percentage text
        QString text = QString::asprintf("%.1f%%", percent);
        QFont f = option.font;
        f.setPointSize(8);
        f.setWeight(QFont::DemiBold);
        painter->setFont(f);

        // Subtle drop shadow for readability
        painter->setPen(QColor(0, 0, 0, 180));
        painter->drawText(option.rect.adjusted(1, 1, 1, 1), Qt::AlignCenter, text);

        painter->setPen(tokens.textPrimary);
        painter->drawText(option.rect, Qt::AlignCenter, text);
    }

    painter->restore();
}

QSize SizeBarDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(80, 24);
}
