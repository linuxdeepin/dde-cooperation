#ifndef MAINWINDOW_P_H
#define MAINWINDOW_P_H

#include <QStackedLayout>

#ifdef WIN32
class QPaintEvent;
#endif

namespace data_transfer_core {

class MainWindow;
class MainWindowPrivate {
    friend class MainWindow;

public:
    explicit MainWindowPrivate(MainWindow *qq);
    virtual ~MainWindowPrivate();

protected:
    void initWindow();
    void initWidgets();
    void moveCenter();

#ifdef WIN32
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *event);
#endif

protected:
    MainWindow *q { nullptr };
    QStackedLayout *mainLayout { nullptr };

#ifdef WIN32
    QLayout *windowsCentralWidget { nullptr};
    QPoint  lastPosition;
    bool  leftButtonPressed { false };
#endif

};

}
#endif // MAINWINDOW_P_H
