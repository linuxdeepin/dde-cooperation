#ifndef READYWIDGET_H
#define READYWIDGET_H

#include <QFrame>

class QLineEdit;
class QToolButton;
class ReadyWidget : public QFrame
{
public:
    ReadyWidget(QWidget *parent = nullptr);
    ~ReadyWidget();

public slots:
    void nextPage();
    void backPage();
    void onLineTextChange();
private:
    void initUI();
    void updateOption();

    QLineEdit *ipInput{ nullptr };
    QLineEdit *captchaInput{ nullptr };
    QToolButton *nextButton{ nullptr };
};

#endif // READYWIDGET_H
