#ifndef ZIPFILEPROCESSRESULTWIDGET_H
#define ZIPFILEPROCESSRESULTWIDGET_H

#include <QFrame>
class zipFileProcessResultWidget : public QFrame
{
public:
    zipFileProcessResultWidget(QWidget *parent = nullptr);
    ~zipFileProcessResultWidget();

private:
    void initUI();

    void successed();
    void failed();
private slots:
    void backPage();
    void informationPage();
    void exit();

};

#endif // ZIPFILEPROCESSRESULTWIDGET_H
