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
};

#endif // ERRORWIDGET_H
