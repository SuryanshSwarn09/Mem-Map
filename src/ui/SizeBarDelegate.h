#pragma once

#include <QStyledItemDelegate>

class SizeBarDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit SizeBarDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
