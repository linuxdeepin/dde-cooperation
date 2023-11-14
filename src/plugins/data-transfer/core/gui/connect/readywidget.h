#ifndef READYWIDGET_H
#define READYWIDGET_H

#include <QFrame>

class QLineEdit;
class QToolButton;
class QLabel;
class ReadyWidget : public QFrame
{
public:
    ReadyWidget(QWidget *parent = nullptr);
    ~ReadyWidget();

public slots:
    void nextPage();
    void backPage();
    void onLineTextChange();
    void connectFailed();
private:
    void initUI();
    void tryConnect();
    void setnextButEnable(bool enabel);
    QLineEdit *ipInput{ nullptr };
    QLineEdit *captchaInput{ nullptr };
    QToolButton *nextButton{ nullptr };
    QLabel *tiptextlabel{ nullptr };

    QTimer *timer{ nullptr };
};

#endif // READYWIDGET_H
