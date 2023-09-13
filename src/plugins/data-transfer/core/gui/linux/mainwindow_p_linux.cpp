#include "../mainwindow.h"
#include "../mainwindow_p.h"

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

using namespace data_transfer_core;

void MainWindowPrivate::initWindow()
{
    q->setWindowTitle("数据迁移");
    q->setFixedSize(800, 500);
    q->setWindowIcon(QIcon::fromTheme("folder"));

    QLabel *label = new QLabel(q);
    label->setPixmap(QIcon::fromTheme("folder").pixmap(40, 40));
    label->setAlignment(Qt::AlignLeft);
    label->setContentsMargins(10, 3, 3, 3);
    q->titlebar()->addWidget(label, Qt::AlignLeft);

    QWidget *centerWidget = new QWidget(q);
    QVBoxLayout *layout = new QVBoxLayout(centerWidget);
    centerWidget->setLayout(layout);
    layout->setContentsMargins(8, 8, 8, 8);

    q->setCentralWidget(centerWidget);
}


void MainWindowPrivate::initWidgets()
{
    StartWidget *startwidget = new StartWidget(q);
    SearchWidget *searchwidget = new SearchWidget(q);
    ConnectWidget *connectwidget = new ConnectWidget(q);
    FileTransWidget *filetranswidget = new FileTransWidget(q);
    AppTransWidget *apptranswidget = new AppTransWidget(q);
    ConfigTransWidget *configtranswidget = new ConfigTransWidget(q);
    TransferringWidget *transferringwidget = new TransferringWidget(q);
    SuccessWidget *successtranswidget = new SuccessWidget(q);

    QStackedWidget *stackedWidget = new QStackedWidget(q);
    stackedWidget->addWidget(startwidget);
    stackedWidget->addWidget(searchwidget);
    stackedWidget->addWidget(connectwidget);
    stackedWidget->addWidget(filetranswidget);
    stackedWidget->addWidget(apptranswidget);
    stackedWidget->addWidget(configtranswidget);
    stackedWidget->addWidget(transferringwidget);
    stackedWidget->addWidget(successtranswidget);
    stackedWidget->setCurrentIndex(0);

    q->centralWidget()->layout()->addWidget(stackedWidget);
}

