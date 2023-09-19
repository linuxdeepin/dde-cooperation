#ifndef CONFIGSELECTWIDGET_H
#define CONFIGSELECTWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>

class ConfigSelectWidget : public QFrame
{
    Q_OBJECT

public:
    ConfigSelectWidget(QWidget *parent = nullptr);
    ~ConfigSelectWidget();

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
