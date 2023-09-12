#ifndef APPTRANSWIDGET_H
#define APPTRANSWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QIcon>
#include <QLabel>

class AppTransWidget : public QFrame
{
    Q_OBJECT

public:
    AppTransWidget(QWidget *parent = nullptr);
    ~AppTransWidget();

public slots:
    void nextPage();

private:
    void initUI();
    void initSelectFrame();
    QFrame *selectFrame = nullptr;
};

#endif
