#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QFrame>
#include <QHBoxLayout>

class SearchWidget : public QFrame
{
    Q_OBJECT

public:
    SearchWidget(QWidget *parent = nullptr);
    ~SearchWidget();

public slots:
    void nextPage();

private:
    QGridLayout *userlayout = nullptr;
    void initUI();
    void initUserlayout();
};

#endif
