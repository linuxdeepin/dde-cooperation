#include "mainwindow.h"
#include "startwidget.h"
#include "searchwidget.h"
#include "connectwidget.h"
#include "filetranswidget.h"
#include "apptranswidget.h"
#include "configtranswidget.h"
#include "transferringwidget.h"
#include "successwidget.h"

#include <DTitlebar>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>
#include <QDesktopWidget>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    setWindowTitle("数据迁移");
    setFixedSize(800, 500);
    setWindowIcon(QIcon::fromTheme("folder"));

    QLabel *label = new QLabel(this);
    label->setPixmap(QIcon::fromTheme("folder").pixmap(40, 40));
    label->setAlignment(Qt::AlignLeft);
    label->setContentsMargins(10, 3, 3, 3);
    titlebar()->addWidget(label, Qt::AlignLeft);

    QWidget *centerWidget = new QWidget(this);
    layout = new QVBoxLayout(centerWidget);
    centerWidget->setLayout(layout);
    layout->setContentsMargins(8, 8, 8, 8);

    setCentralWidget(centerWidget);
    initWidget();
}

MainWindow::~MainWindow()
{
}

void MainWindow::moveCenter()
{
    QScreen *cursorScreen = nullptr;
    const QPoint &cursorPos = QCursor::pos();

    QList<QScreen *> screens = qApp->screens();
    QList<QScreen *>::const_iterator it = screens.begin();
    for (; it != screens.end(); ++it) {
        if ((*it)->geometry().contains(cursorPos)) {
            cursorScreen = *it;
            break;
        }
    }

    if (!cursorScreen)
        cursorScreen = qApp->primaryScreen();
    if (!cursorScreen)
        return;

    int x = (cursorScreen->availableGeometry().width() - width()) / 2;
    int y = (cursorScreen->availableGeometry().height() - height()) / 2;
    move(QPoint(x, y) + cursorScreen->geometry().topLeft());
}

void MainWindow::initWidget()
{
    StartWidget *startwidget = new StartWidget(this);
    SearchWidget *searchwidget = new SearchWidget(this);
    ConnectWidget *connectwidget = new ConnectWidget(this);
    FileTransWidget *filetranswidget = new FileTransWidget(this);
    AppTransWidget *apptranswidget = new AppTransWidget(this);
    ConfigTransWidget *configtranswidget = new ConfigTransWidget(this);
    TransferringWidget *transferringwidget = new TransferringWidget(this);
    SuccessWidget *successtranswidget = new SuccessWidget(this);

    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(startwidget);
    stackedWidget->addWidget(searchwidget);
    stackedWidget->addWidget(connectwidget);
    stackedWidget->addWidget(filetranswidget);
    stackedWidget->addWidget(apptranswidget);
    stackedWidget->addWidget(configtranswidget);
    stackedWidget->addWidget(transferringwidget);
    stackedWidget->addWidget(successtranswidget);
    stackedWidget->setCurrentIndex(0);

    layout->addWidget(stackedWidget);
}
