#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include <QStackedLayout>
#include <QDockWidget>
#ifdef WIN32
class QPaintEvent;
#endif

namespace data_transfer_core {

enum PageName {
    startwidget = 0,
    choosewidget = 1,
    promptwidget = 2,
    readywidget = 3,
    selectmainwidget = 4,
    transferringwidget = 5,
    successtranswidget = 6,
    filewselectidget = 7,
    configselectwidget = 8,
    appselectwidget = 9
};
class MainWindow;
class MainWindowPrivate : public QObject
{
    Q_OBJECT
    friend class MainWindow;

public:
    explicit MainWindowPrivate(MainWindow *qq);
    virtual ~MainWindowPrivate();

protected:
    void initWindow();
    void initSideBar();
    void initWidgets();
    void moveCenter();

private slots:
    void handleCurrentChanged(int index);

#ifdef WIN32
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *event);

    void initTitleBar();
#endif

protected:
    MainWindow *q{ nullptr };
    QStackedLayout *mainLayout{ nullptr };
    QDockWidget *sidebar{ nullptr };

#ifdef WIN32
    QHBoxLayout *windowsCentralWidget{ nullptr };
    QHBoxLayout *windowsCentralWidgetContent{ nullptr };
    QHBoxLayout *windowsCentralWidgetSidebar{ nullptr };
    QPoint lastPosition;
    bool leftButtonPressed{ false };
#endif
};

} // namespace data_transfer_core
#endif // MAINWINDOW_P_H
