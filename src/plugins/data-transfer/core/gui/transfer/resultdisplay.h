#ifndef RESULTDISPLAYWIDGET_H
#define RESULTDISPLAYWIDGET_H

#include <QFrame>
#include <QTextBrowser>

class ResultDisplayWidget : public QFrame
{
    Q_OBJECT

public:
    ResultDisplayWidget(QWidget *parent = nullptr);
    ~ResultDisplayWidget();

public slots:
    void themeChanged(int theme);

private:
    void initUI();
    void nextPage();

private:
    QTextBrowser *processTextBrowser { nullptr };
};

#endif
