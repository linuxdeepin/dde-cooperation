#ifndef WAITTRANSFERRWIDGET_H
#define WAITTRANSFERRWIDGET_H

#include <QFrame>
#include <QHBoxLayout>

class WaitTransferWidget : public QFrame
{
    Q_OBJECT

public:
    WaitTransferWidget(QWidget *parent = nullptr);
    ~WaitTransferWidget();

public slots:
    void nextPage();
    void backPage();

private:
    void initUI();
};

#endif
