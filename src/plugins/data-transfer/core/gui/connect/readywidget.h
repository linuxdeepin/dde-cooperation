#ifndef READYWIDGET_H
#define READYWIDGET_H

#include <QFrame>

class ReadyWidget : public QFrame
{
public:
    ReadyWidget(QWidget *parent = nullptr);
    ~ReadyWidget();

public slots:
    void nextPage();
    void backPage();
public:
    void initUI();
};

#endif // READYWIDGET_H
