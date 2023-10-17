#ifndef SUCCESSWIDGET_H
#define SUCCESSWIDGET_H

#include <QFrame>
#include <QTextBrowser>

class SuccessWidget : public QFrame
{
    Q_OBJECT

public:
    SuccessWidget(QWidget *parent = nullptr);
    ~SuccessWidget();

public slots:
    void themeChanged(int theme);

private:
    void initUI();
    void nextPage();

private:
    QTextBrowser *processTextBrowser { nullptr };
};

#endif
