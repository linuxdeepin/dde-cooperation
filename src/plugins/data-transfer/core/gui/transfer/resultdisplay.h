#ifndef RESULTDISPLAYWIDGET_H
#define RESULTDISPLAYWIDGET_H

#include <QFrame>
#include <QItemDelegate>
#include <QListView>

class ResultDisplayWidget : public QFrame
{
    Q_OBJECT

public:
    ResultDisplayWidget(QWidget *parent = nullptr);
    ~ResultDisplayWidget();

public slots:
    void themeChanged(int theme);
    void addFailure(QString name, QString type, QString reason);

private:
    void initUI();
    void initListTitle();
    void initListView();
    void nextPage();

private:
    QListView *listview { nullptr };
    QFrame *listTitle { nullptr };
};

class itemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    itemDelegate();
    ~itemDelegate() override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintText(QPainter *painter, const QStyleOptionViewItem &option,
                   const QModelIndex &index) const;
    void paintBackground(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const;
};

#endif
