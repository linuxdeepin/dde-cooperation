#include "item.h"

#include <QApplication>
#include <QLabel>
#include <QPainterPath>

ItemTitlebar::ItemTitlebar(const QString &label1_, const QString &label2_,
                           const qreal &label1LeftMargin_, const qreal &label2LeftMargin_,
                           const QRectF &iconPosSize_, const qreal &iconRadius_, QWidget *parent)
    : QFrame(parent),
      label1(label1_),
      label2(label2_),
      label1LeftMargin(label1LeftMargin_),
      label2LeftMargin(label2LeftMargin_),
      iconPosSize(iconPosSize_),
      iconRadius(iconRadius_)
{
    initUI();
}

ItemTitlebar::ItemTitlebar()
{
    initUI();
}

ItemTitlebar::~ItemTitlebar() { }

void ItemTitlebar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path1;
    path1.addRoundedRect(iconPosSize, iconRadius, iconRadius);
    painter.setPen(QPen(QColor(65, 77, 104), 2));
    painter.drawPath(path1);

    painter.setPen(QPen(QColor(0, 26, 46), 2));
    QPainterPath path2;
    path2.moveTo(iconPosSize.x() + 4, iconPosSize.y() + 8);
    path2.lineTo(iconPosSize.x() + 12, iconPosSize.y() + 8);
    painter.drawPath(path2);

    painter.setPen(QPen(QColor(0, 0, 0, 50), 1));
    QPainterPath path3;
    path3.moveTo(label1LeftMargin, 5);
    path3.lineTo(label1LeftMargin, 32);
    painter.drawPath(path3);

    QPainterPath path4;
    path4.moveTo(label2LeftMargin, 5);
    path4.lineTo(label2LeftMargin, 32);
    painter.drawPath(path4);
}

void ItemTitlebar::initUI()
{
    setProperty("class", "myqframe");
    setContentsMargins(0, 0, 0, 0);
    setStyleSheet(".QLabel{"
                  "color: rgba(65,77,104,1);"
                  "opacity: 1;"
                  "font-family: \"SourceHanSansSC-Medium\";"
                  "font-size: 14px;"
                  "font-weight: 500;"
                  "font-style: normal;"
                  "letter-spacing: 0px;"
                  "text-align: left;}"
                  ".myqframe{"
                  "border: none;"
                  "border-radius: 8px;"
                  "}");
    QLabel *title1 = new QLabel(label1, this);
    QLabel *title2 = new QLabel(label2, this);

    title1->setGeometry(label1LeftMargin + 10, 8, 177.3, 20);
    title2->setGeometry(label2LeftMargin + 10, 8, 61.3, 20);
}

void ItemTitlebar::setIconRadius(qreal newIconRadius)
{
    iconRadius = newIconRadius;
}

void ItemTitlebar::setIconPosSize(const QRectF &newIconPosSize)
{
    iconPosSize = newIconPosSize;
}

void ItemTitlebar::setLabel2LeftMargin(qreal newLabel2LeftMargin)
{
    label2LeftMargin = newLabel2LeftMargin;
}

void ItemTitlebar::setLabel2(const QString &newLabel2)
{
    label2 = newLabel2;
}

void ItemTitlebar::setLabel1LeftMargin(qreal newLabel1LeftMargin)
{
    label1LeftMargin = newLabel1LeftMargin;
}

void ItemTitlebar::setLabel1(const QString &newLabel1)
{
    label1 = newLabel1;
}

ItemDelegate::ItemDelegate() { }

ItemDelegate::ItemDelegate(const qreal &filenameTextLeftMargin_, const qreal &filenameTextMaxLen_,
                           const qreal &remarkTextLeftMargin_, const qreal &remarkTextMaxLen_,
                           const qreal &backgroundColorLeftMargin_, const QPoint &iconPos_,
                           const QPoint &checkBoxPos_)
    : remarkTextLeftMargin(remarkTextLeftMargin_),
      remarkTextMaxLen(remarkTextMaxLen_),
      filenameTextLeftMargin(filenameTextLeftMargin_),
      filenameTextMaxLen(filenameTextMaxLen_),
      backgroundColorLeftMargin(backgroundColorLeftMargin_),
      iconPos(iconPos_),
      checkBoxPos(checkBoxPos_)
{
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    // draw back
    paintBackground(painter, option, index);

    // draw icon
    paintIcon(painter, option, index);

    // draw text
    paintText(painter, option, index);

    // draw checkbox
    paintCheckbox(painter, option, index);
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(480, 36);
}

void ItemDelegate::paintIcon(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    painter->save();
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull()) {
        QPoint pos = option.rect.topLeft();
        pos += iconPos;

        QRect positon = QRect(pos, QSize(24, 24));
        icon.paint(painter, positon, Qt::AlignLeft | Qt::AlignVCenter);
    }
    painter->restore();
}

void ItemDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    painter->save();

    QColor evencolor(0, 0, 0, 30);

    painter->setPen(Qt::NoPen);

    QPoint topleft = option.rect.topLeft();
    QRect positon(backgroundColorLeftMargin, topleft.y(), 440,
                  36); //= option.rect.adjusted(backgroundColorLeftMargin, 0, 0, 0);
    if (index.row() % 2 != 0) {
        painter->setBrush(evencolor);
        painter->drawRoundedRect(positon, 10, 10);
    }
    painter->restore();
}

void ItemDelegate::paintText(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    painter->save();

    QRect filenameTextPos = option.rect.adjusted(filenameTextLeftMargin, 0, 0, 0);

    QFont filenameTextFont;
    filenameTextFont.setPixelSize(12);
    QString filenameText = index.data(Qt::DisplayRole).toString();
    QFontMetrics filenameMetrics(filenameTextFont);
    filenameText = filenameMetrics.elidedText(filenameText, Qt::ElideRight, filenameTextMaxLen);
    painter->drawText(filenameTextPos, Qt::AlignLeft | Qt::AlignVCenter, filenameText);

    QRect remarkTextPos = option.rect.adjusted(remarkTextLeftMargin, 0, 0, 0);
    QFont remarkTextFont;
    remarkTextFont.setPixelSize(12);
    QString remarkText = index.data(Qt::ToolTipRole).toString();
    QFontMetrics remarkMetrics(remarkTextFont);
    remarkTextFont = remarkMetrics.elidedText(filenameText, Qt::ElideRight, remarkTextMaxLen);
    painter->drawText(remarkTextPos, Qt::AlignLeft | Qt::AlignVCenter, remarkText);

    painter->restore();
}

void ItemDelegate::paintCheckbox(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    painter->save();
    QPoint pos = option.rect.topLeft();
    pos = pos + checkBoxPos;
    QItemDelegate::drawCheck(painter, option, QRect(pos, QSize(18, 18)),
                             index.data(Qt::CheckStateRole).value<Qt::CheckState>());
    painter->restore();
}

void ItemDelegate::setCheckBoxPos(QPoint newCheckBoxPos)
{
    checkBoxPos = newCheckBoxPos;
}

void ItemDelegate::setIconPos(QPoint newIconPos)
{
    iconPos = newIconPos;
}

void ItemDelegate::setBackgroundColorLeftMargin(qreal newBackgroundColorLeftMargin)
{
    backgroundColorLeftMargin = newBackgroundColorLeftMargin;
}

void ItemDelegate::setFilenameTextMaxLen(qreal newFilenameTextMaxLen)
{
    filenameTextMaxLen = newFilenameTextMaxLen;
}

void ItemDelegate::setFilenameTextLeftMargin(qreal newFilenameTextLeftMargin)
{
    filenameTextLeftMargin = newFilenameTextLeftMargin;
}

void ItemDelegate::setRemarkTextMaxLen(qreal newRemarkTextMaxLen)
{
    remarkTextMaxLen = newRemarkTextMaxLen;
}

void ItemDelegate::setRemarkTextLeftMargin(qreal newRemarkTextLeftMargin)
{
    remarkTextLeftMargin = newRemarkTextLeftMargin;
}
