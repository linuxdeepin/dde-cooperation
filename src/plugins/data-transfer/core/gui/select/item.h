#ifndef SELECTITEM_H
#define SELECTITEM_H

#include <QCheckBox>
#include <QItemDelegate>
#include <QPainter>
#include <QStandardItemModel>
#include <QFrame>
#include <QListView>
#include <QModelIndex>
#include <QMouseEvent>

class SelectAllButton : public QFrame
{
    Q_OBJECT
public:
    SelectAllButton(QWidget *parent = nullptr);
    ~SelectAllButton();
protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event)override;
signals:
    void selectAll();
private:
    QRectF iconPosSize{ 2, 2, 16, 16 };
    qreal iconRadius{ 3 };
};
class ItemTitlebar : public QFrame
{
    Q_OBJECT
public:
    ItemTitlebar(const QString &label1_, const QString &label2_, const qreal &label1LeftMargin_,
                 const qreal &label2LeftMargin_, const QRectF &iconPosSize_,
                 const qreal &iconRadius_, QWidget *parent = nullptr);
    ItemTitlebar(QWidget *parent = nullptr);
    ~ItemTitlebar();

    void setLabel1(const QString &newLabel1);

    void setLabel1LeftMargin(qreal newLabel1LeftMargin);

    void setLabel2(const QString &newLabel2);

    void setLabel2LeftMargin(qreal newLabel2LeftMargin);

    void setIconPosSize(const QRectF &newIconPosSize);

    void setIconRadius(qreal newIconRadius);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void selectAll();

private:
    void initUI();

private:
    QString label1{ "表项一" };
    qreal label1LeftMargin{ 50 };
    QString label2{ "表项二" };
    qreal label2LeftMargin{ 360 };
    QRectF iconPosSize{ 10, 12, 16, 16 };
    qreal iconRadius{ 3 };

    SelectAllButton *selectAllButton{ nullptr };
};


class ItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    ItemDelegate();
    ItemDelegate(const qreal &filenameTextLeftMargin_, const qreal &filenameTextMaxLen_,
                 const qreal &remarkTextLeftMargin_, const qreal &remarkTextMaxLen_,
                 const qreal &backgroundColorLeftMargin_, const QPoint &iconPos_,
                 const QPoint &checkBoxPos_);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setRemarkTextLeftMargin(qreal newRemarkTextLeftMargin);

    void setRemarkTextMaxLen(qreal newRemarkTextMaxLen);

    void setFilenameTextLeftMargin(qreal newFilenameTextLeftMargin);

    void setFilenameTextMaxLen(qreal newFilenameTextMaxLen);

    void setBackgroundColorLeftMargin(qreal newBackgroundColorLeftMargin);

    void setIconPos(QPoint newIconPos);

    void setCheckBoxPos(QPoint newCheckBoxPos);

private:
    void paintIcon(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    void paintText(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintCheckbox(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

private:
    qreal remarkTextLeftMargin{ 379 };
    qreal remarkTextMaxLen{ 100 };

    qreal filenameTextLeftMargin{ 99 };
    qreal filenameTextMaxLen{ 250 };

    qreal backgroundColorLeftMargin{ 50 };

    QPoint iconPos{ 65, 6 };
    QPoint checkBoxPos{ 10, 9 };
};

class SaveItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SaveItemDelegate();
    ~SaveItemDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintIcon(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    void paintText(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintCheckbox(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

private:
    qreal remarkTextLeftMargin{ 290 };
    qreal remarkTextMaxLen{ 100 };

    qreal filenameTextLeftMargin{ 52 };
    qreal filenameTextMaxLen{ 250 };

    qreal backgroundColorLeftMargin{ 0 };

    QPoint iconPos{ 14, 10 };
    QPoint checkBoxPos{ 0, 0 };
};

class SidebarItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SidebarItemDelegate();
    ~SidebarItemDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintText(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintCheckbox(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;
};

class SelectListView : public QListView
{
    Q_OBJECT
public:
    SelectListView(QFrame *parent = nullptr);
    ~SelectListView();

private:
    bool isSelectAll{false};
public slots:
    void selectorDelAllItem();
};
#endif
