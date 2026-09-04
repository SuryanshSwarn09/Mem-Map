#include "SizeBarDelegate.h"
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

    // Selected or hovered background
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    // Bar dimensions
    int padX = 6;
    int padY = 5;
    QRect barRect = option.rect.adjusted(padX, padY, -padX, -padY);

    if (barRect.width() > 10 && barRect.height() > 4) {
        // Track background
        QPainterPath trackPath;
        trackPath.addRoundedRect(barRect, 3, 3);
        painter->fillPath(trackPath, QColor(45, 52, 64, 180));

        // Fill bar
        int fillWidth = static_cast<int>(barRect.width() * (qBound(0.0, percent, 100.0) / 100.0));
        if (fillWidth > 2) {
            QRect filledRect(barRect.x(), barRect.y(), fillWidth, barRect.height());
            QPainterPath fillPath;
            fillPath.addRoundedRect(filledRect, 3, 3);

            QLinearGradient grad(filledRect.topLeft(), filledRect.topRight());
            if (percent > 70.0) {
                grad.setColorAt(0.0, QColor(244, 67, 54));   // Red
                grad.setColorAt(1.0, QColor(255, 87, 34));
            } else if (percent > 35.0) {
                grad.setColorAt(0.0, QColor(255, 152, 0));  // Orange
                grad.setColorAt(1.0, QColor(255, 193, 7));
            } else {
                grad.setColorAt(0.0, QColor(33, 150, 243));  // Blue
                grad.setColorAt(1.0, QColor(0, 188, 212));   // Cyan
            }

            painter->fillPath(fillPath, grad);
        }

        // Percentage text
        QString text = QString::asprintf("%.1f%%", percent);
        painter->setPen(QColor(235, 240, 245));
        QFont f = option.font;
        f.setPointSize(8);
        painter->setFont(f);
        painter->drawText(option.rect, Qt::AlignCenter, text);
    }

    painter->restore();
}

QSize SizeBarDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(80, 24);
}
