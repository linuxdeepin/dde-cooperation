#ifndef APPSELECTWIDGET_H
#define APPSELECTWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>

class AppSelectWidget : public QFrame
{
    Q_OBJECT

public:
    AppSelectWidget(QWidget *parent = nullptr);
    ~AppSelectWidget();

public slots:
    void nextPage();

private:
    void initUI();
    void initSelectFrame();
    void sendOptions();
    QFrame *selectFrame { nullptr };
    QGridLayout *selectLayout { nullptr };
};

#endif
