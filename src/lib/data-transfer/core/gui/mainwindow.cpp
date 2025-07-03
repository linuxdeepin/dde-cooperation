#include "mainwindow.h"
#include "mainwindow_p.h"
#include "common/log.h"

#include <QScreen>
#include <QApplication>

using namespace data_transfer_core;

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags /*flags*/)
    : CrossMainWindow(parent), d(new MainWindowPrivate(this))
{
    DLOG << "Initializing main window";

    d->initWindow();
    d->initWidgets();
    d->moveCenter();
    DLOG << "Main window initialized";
}

MainWindow::~MainWindow()
{
    DLOG << "Destroying main window";
}

#if defined(_WIN32) || defined(_WIN64)
void MainWindow::paintEvent(QPaintEvent *event)
{
    DLOG << "Paint event triggered (Windows)";
    d->paintEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    DLOG << "Close event triggered (Windows), quitting application";
    QApplication::quit();
}
#endif

MainWindowPrivate::MainWindowPrivate(MainWindow *qq) : q(qq)
{
    DLOG << "MainWindowPrivate init";
}

MainWindowPrivate::~MainWindowPrivate()
{
    DLOG << "MainWindowPrivate destroy";
}

void MainWindowPrivate::moveCenter()
{
    DLOG << "Moving window to center of current screen";

    QScreen *cursorScreen = nullptr;
    const QPoint &cursorPos = QCursor::pos();

    QList<QScreen *> screens = qApp->screens();
    DLOG << "Found" << screens.size() << "available screens";
    QList<QScreen *>::const_iterator it = screens.begin();
    for (; it != screens.end(); ++it) {
        if ((*it)->geometry().contains(cursorPos)) {
            DLOG << "Cursor is on this screen:" << (*it)->name().toStdString();
            cursorScreen = *it;
            break;
        }
    }

    if (!cursorScreen) {
        DLOG << "No screen contains cursor, using primary screen";
        cursorScreen = qApp->primaryScreen();
    }
    if (!cursorScreen) {
        DLOG << "No primary screen found, cannot center window";
        return;
    }

    int x = (cursorScreen->availableGeometry().width() - q->width()) / 2;
    int y = (cursorScreen->availableGeometry().height() - q->height()) / 2;
    q->move(QPoint(x, y) + cursorScreen->geometry().topLeft());
    DLOG << "Calculated center position - x:" << x << "y:" << y
             << "on screen:" << cursorScreen->name().toStdString();
}

