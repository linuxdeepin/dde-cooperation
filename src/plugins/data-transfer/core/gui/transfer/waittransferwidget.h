#ifndef WAITTRANSFERRWIDGET_H
#define WAITTRANSFERRWIDGET_H

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

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
    QToolButton *backButton { nullptr };
    QLabel *iconLabel { nullptr };
    QMovie *lighticonmovie { nullptr };
    QMovie *darkiconmovie { nullptr };
};

#endif
