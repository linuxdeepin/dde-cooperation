#ifndef RESULTDISPLAYWIDGET_H
#define RESULTDISPLAYWIDGET_H

#include <QFrame>
#include <QListView>

class ResultDisplayWidget : public QFrame
{
    Q_OBJECT

public:
    ResultDisplayWidget(QWidget *parent = nullptr);
    ~ResultDisplayWidget();

public slots:
    void themeChanged(int theme);

private:
    void initUI();
    void initListTitle();
    void initListView();
    void nextPage();

private:
    QListView *listview { nullptr };
    QFrame *listTitle { nullptr };
};

#endif
