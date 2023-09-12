#ifndef CONFIGTRANSWIDGET_H
#define CONFIGTRANSWIDGET_H

#include <QCheckBox>
#include <QFrame>
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
    QFrame *selectFrame = nullptr;
};

#endif
