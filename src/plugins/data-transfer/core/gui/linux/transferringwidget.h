#ifndef TRANSFERRWIDGET_H
#define TRANSFERRWIDGET_H

#include <QFrame>
#include <QHBoxLayout>

class TransferringWidget : public QFrame
{
    Q_OBJECT

public:
    TransferringWidget(QWidget *parent = nullptr);
    ~TransferringWidget();

public slots:
    void nextPage();

private:
    void initUI();
};

#endif
