#ifndef SUCCESSWIDGET_H
#define SUCCESSWIDGET_H

#include <QFrame>

class SuccessWidget : public QFrame
{
    Q_OBJECT

public:
    SuccessWidget(QWidget *parent = nullptr);
    ~SuccessWidget();

private:
    void initUI();
};

#endif
