#ifndef PROMPTWIDGET_H
#define PROMPTWIDGET_H

#include <QFrame>
class QToolButton;
class PromptWidget : public QFrame
{
    Q_OBJECT

public:
    PromptWidget(QWidget *parent = nullptr);
    ~PromptWidget();

public slots:
    void nextPage();
    void backPage();
    void themeChanged(int theme);

private:
    void initUI();
    QToolButton *backButton{nullptr};
};

#endif
