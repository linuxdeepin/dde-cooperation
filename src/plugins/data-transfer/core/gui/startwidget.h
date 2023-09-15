#ifndef STARTWIDGET_H
#define STARTWIDGET_H

#include <QFrame>

class StartWidget : public QFrame
{
    Q_OBJECT

public:
    StartWidget(QWidget *parent = nullptr);
    ~StartWidget();

public slots:
    void nextPage();

private:
    void initUI();
};

#endif
