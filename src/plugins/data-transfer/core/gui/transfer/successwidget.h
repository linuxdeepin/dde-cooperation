#ifndef SUCCESSWIDGET_H
#define SUCCESSWIDGET_H

#include <QFrame>

class SuccessWidget : public QFrame
{
    Q_OBJECT

public:
    SuccessWidget(QWidget *parent = nullptr);
    ~SuccessWidget();

    void backPage();

private:
    void initUI();
};

#endif
