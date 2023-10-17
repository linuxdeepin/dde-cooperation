#ifndef STARTWIDGET_H
#define STARTWIDGET_H

#include <QCheckBox>
#include <QFrame>
#include <QToolButton>

class StartWidget : public QFrame
{
    Q_OBJECT

public:
    StartWidget(QWidget *parent = nullptr);
    ~StartWidget();

public slots:
    void nextPage();
    void themeChanged(int theme);

private:
    void initUI();
    QCheckBox *checkBox { nullptr };
    QToolButton *nextButton { nullptr };
};

#endif
