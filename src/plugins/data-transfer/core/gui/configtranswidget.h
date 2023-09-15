#ifndef CONFIGTRANSWIDGET_H
#define CONFIGTRANSWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>

class ConfigTransWidget : public QFrame
{
    Q_OBJECT

public:
    ConfigTransWidget(QWidget *parent = nullptr);
    ~ConfigTransWidget();

public slots:
    void nextPage();

private:
    void initUI();
    void initSelectFrame();
    void sendOptions();

private:
    QFrame *selectFrame { nullptr };
    QGridLayout *selectLayout { nullptr };
};

#endif
