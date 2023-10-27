#ifndef CONFIGSELECTWIDGET_H
#define CONFIGSELECTWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>

#include "../select/selectmainwidget.h"
class SelectTitlebar;
class QToolButton;
class QListView;
class SelectListView;
class ConfigSelectWidget : public QFrame
{
    Q_OBJECT
public:
    ConfigSelectWidget(QWidget *parent = nullptr);
    ~ConfigSelectWidget();
    void changeText();
public slots:
    void nextPage();
    void backPage();
signals:
    void isOk(const SelectItemName &name);

private:
    void initUI();
    void initSelectBrowerBookMarkFrame();
    void initSelectConfigFrame();
    void sendOptions();
    void delOptions();
private:
    QFrame *selectBrowerBookMarkFrame{ nullptr };
    QFrame *selectConfigFrame{ nullptr };

    SelectListView *browserView{ nullptr };
    SelectListView *configView{ nullptr };

    QVBoxLayout *selectMainLayout{ nullptr };
    QToolButton *determineButton{ nullptr };
    QToolButton *cancelButton{ nullptr };

    QLabel *titileLabel{ nullptr };
};

#endif
