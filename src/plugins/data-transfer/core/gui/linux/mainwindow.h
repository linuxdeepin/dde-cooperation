#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <QHBoxLayout>
#include <QStackedWidget>
DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void moveCenter();

private:
    void initWidget();
    QVBoxLayout *layout = nullptr;
    QStackedWidget *stackedWidget = nullptr;
};

#endif   // MAINWINDOW_H
