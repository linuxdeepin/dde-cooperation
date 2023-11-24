#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include <QStackedLayout>
#include <QDockWidget>
#include <QStackedWidget>
#ifdef WIN32
class QPaintEvent;
#endif

namespace data_transfer_core {

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
    void initWidgets();
    void moveCenter();

private slots:
    void handleCurrentChanged(int index);
    void clearWidget();
    void changeAllWidgtText();
#ifdef WIN32
protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    void initTitleBar();
    void initSideBar();
#endif

protected:
    MainWindow *q{ nullptr };
    QStackedLayout *mainLayout{ nullptr };
    QDockWidget *sidebar{ nullptr };
    QStackedWidget *stackedWidget{ nullptr };
#ifdef WIN32

protected:
    QHBoxLayout *windowsCentralWidget{ nullptr };
    QHBoxLayout *windowsCentralWidgetContent{ nullptr };
    QHBoxLayout *windowsCentralWidgetSidebar{ nullptr };

    QPoint lastPosition;
    bool leftButtonPressed{ false };
#endif
};

} // namespace data_transfer_core
#endif // MAINWINDOW_P_H
