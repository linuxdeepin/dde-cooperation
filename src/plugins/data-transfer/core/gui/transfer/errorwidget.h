#ifndef ERRORWIDGET_H
#define ERRORWIDGET_H

#include <QFrame>
class ErrorWidget : public QFrame
{
    Q_OBJECT
public:
    ErrorWidget(QWidget *parent = nullptr);
    ~ErrorWidget();

private:
    void initUI();
public slots:
    void backPage();
    void retryPage();

private:
    qreal state{ 1 }; // 1 interneterror，2 transfererror
};

#endif // ERRORWIDGET_H
