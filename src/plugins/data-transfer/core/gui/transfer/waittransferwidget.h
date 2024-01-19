#ifndef WAITTRANSFERRWIDGET_H
#define WAITTRANSFERRWIDGET_H

#include <QFrame>

class QLabel;
class QHBoxLayout;
class QPushButton;
class WaitTransferWidget : public QFrame
{
    Q_OBJECT

public:
    WaitTransferWidget(QWidget *parent = nullptr);
    ~WaitTransferWidget();

public slots:
    void nextPage();
    void backPage();
    void themeChanged(int theme);

private:
    void initUI();

private:
    QPushButton *backButton { nullptr };
    QLabel *iconLabel { nullptr };
    QMovie *lighticonmovie { nullptr };
    QMovie *darkiconmovie { nullptr };

#ifndef WIN32
public:
    void cancel();
#endif
};

#endif
