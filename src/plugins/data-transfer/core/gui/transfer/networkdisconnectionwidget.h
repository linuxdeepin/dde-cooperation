#ifndef NETWORKDISCONNECTIONWIDGET_H
#define NETWORKDISCONNECTIONWIDGET_H

#include<QFrame>
class NetworkDisconnectionWidget: public QFrame
{
    Q_OBJECT
public:
    NetworkDisconnectionWidget(QWidget *parent = nullptr);
    ~NetworkDisconnectionWidget();

private:
    void initUI();
public slots:
    void backPage();
    void retryPage();
};

#endif // NETWORKDISCONNECTIONWIDGET_H
