#include "item.h"

#include <QApplication>

ItemDelegate::ItemDelegate()
{
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    //draw back
    paintBackground(painter, option, index);

    //draw icon
    paintIcon(painter, option, index);

    //draw text
    paintText(painter, option, index);

    painter->save();
    QPoint pos = option.rect.topLeft();
    pos += QPoint(0, 10);
    QItemDelegate::drawCheck(painter,
                             option,
                             QRect(pos, QSize(17, 17)),
                             index.data(Qt::CheckStateRole).value<Qt::CheckState>());
    painter->restore();
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(480, 36);
}

void ItemDelegate::paintIcon(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        QPoint pos = option.rect.topLeft();
        pos += QPoint(30, 6);

        QRect positon = QRect(pos, QSize(24, 24));
        icon.paint(painter, positon, Qt::AlignLeft | Qt::AlignVCenter);
    }
    painter->restore();
}

void ItemDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QColor evencolor;
    evencolor.setNamedColor("#000000");
    evencolor.setAlpha(12);

    painter->setPen(Qt::NoPen);

    QRect positon = option.rect.adjusted(20, 0, 0, 0);

    if (index.row() % 2 == 0) {
        painter->setBrush(evencolor);
        painter->drawRoundedRect(positon, 10, 10);
    }
    painter->restore();
}

void ItemDelegate::paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QRect pos = option.rect.adjusted(60, 0, 0, 0);

    QFont font;
    font.setPixelSize(12);
    painter->setFont(font);
    QString text = index.data(Qt::DisplayRole).toString();
    painter->drawText(pos, Qt::AlignLeft | Qt::AlignVCenter, text);
    painter->restore();
}
