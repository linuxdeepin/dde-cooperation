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

    void initPassWord();
public slots:
    void nextPage();

private:
    void initUI();
    QGridLayout *passwordLayout = nullptr;
};

#endif
