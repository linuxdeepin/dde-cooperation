#ifndef SELECTITEM_H
#define SELECTITEM_H

#include <QCheckBox>
#include <QItemDelegate>
#include <QPainter>
#include <QStandardItemModel>

class ListItem : public QStandardItem
{
public:
    ListItem()
        : QStandardItem()
    {
        checkBox = new QCheckBox();
        checkBox->setChecked(false);
    }

    QCheckBox *getCheckBox() const
    {
        return checkBox;
    }

private:
    QCheckBox *checkBox;
};

class ItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    ItemDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintIcon(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
};

#endif
