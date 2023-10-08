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
class AppSelectWidget : public QFrame
{
    Q_OBJECT

public:
    AppSelectWidget(QWidget *parent = nullptr);
    ~AppSelectWidget();

public slots:
    void nextPage();
    void backPage();

signals:
    void isOk(const SelectItemName &name, const bool &ok);

private:
    void initUI();
    void initSelectFrame();
    void sendOptions();

    QFrame *selectFrame{ nullptr };
    QListView *fileview{ nullptr };

    QVBoxLayout *selectMainLayout{ nullptr };
    QToolButton *determineButton{ nullptr };
    QToolButton *cancelButton{ nullptr };
};

#endif
