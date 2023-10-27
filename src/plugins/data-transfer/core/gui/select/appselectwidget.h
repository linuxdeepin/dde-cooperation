#ifndef APPSELECTWIDGET_H
#define APPSELECTWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QIcon>
#include <QLabel>

#include "../select/selectmainwidget.h"

class QVBoxLayout;
class QListView;
class QToolButton;
class SelectListView;
class AppSelectWidget : public QFrame
{
    Q_OBJECT

public:
    AppSelectWidget(QWidget *parent = nullptr);
    ~AppSelectWidget();
    void changeText();
public slots:
    void nextPage();
    void backPage();

signals:
    void isOk(const SelectItemName &name);

private:
    void initUI();
    void initSelectFrame();
    void sendOptions();
    void delOptions();
private:
    QFrame *selectFrame{ nullptr };
    SelectListView *appView{ nullptr };

    QVBoxLayout *selectMainLayout{ nullptr };
    QToolButton *determineButton{ nullptr };
    QToolButton *cancelButton{ nullptr };
    QLabel *titileLabel{ nullptr };
};

#endif
