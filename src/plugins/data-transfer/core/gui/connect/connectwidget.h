#ifndef CONNECTWIDGET_H
#define CONNECTWIDGET_H

#include <QFrame>
#include <QHBoxLayout>

class ConnectWidget : public QFrame
{
    Q_OBJECT

public:
    ConnectWidget(QWidget *parent = nullptr);
    ~ConnectWidget();

    void initConnectLayout();
    void updatePassWord();

public slots:
    void nextPage();
    void backPage();

private:
    void initUI();
    QHBoxLayout *connectLayout = nullptr;
    int remainingTime = 300;
};

#endif
