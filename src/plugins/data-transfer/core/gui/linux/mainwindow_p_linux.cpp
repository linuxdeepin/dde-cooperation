#include "../mainwindow.h"
#include "../mainwindow_p.h"

#include <DTitlebar>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>

#include <gui/connect/choosewidget.h>
#include <gui/connect/connectwidget.h>
#include <gui/connect/promptwidget.h>
#include <gui/connect/searchwidget.h>
#include <gui/connect/startwidget.h>

#include <gui/select/appselectwidget.h>
#include <gui/select/configselectwidget.h>
#include <gui/select/fileselectwidget.h>

#include <gui/transfer/successwidget.h>
#include <gui/transfer/transferringwidget.h>
#include <gui/transfer/uploadfilewidget.h>
#include <gui/transfer/waittransferwidget.h>

#include <utils/transferhepler.h>

using namespace data_transfer_core;

void MainWindowPrivate::initWindow()
{
    q->setWindowTitle("数据迁移");
    q->setFixedSize(740, 552);
    q->setWindowIcon(QIcon(":/icon/icon.svg"));

    QLabel *label = new QLabel(q);
    label->setPixmap(QIcon(":/icon/icon.svg").pixmap(30, 30));
    label->setAlignment(Qt::AlignLeft);
    label->setContentsMargins(10, 10, 3, 3);
    q->titlebar()->addWidget(label, Qt::AlignLeft);

    QWidget *centerWidget = new QWidget(q);
    QVBoxLayout *layout = new QVBoxLayout(centerWidget);
    centerWidget->setLayout(layout);
    layout->setContentsMargins(8, 8, 8, 8);

    q->setCentralWidget(centerWidget);

    initSideBar();
}

void MainWindowPrivate::initSideBar()
{
    sidebar = new QDockWidget("Sidebar", q);
    q->setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
    q->addDockWidget(Qt::LeftDockWidgetArea, sidebar);
    sidebar->setTitleBarWidget(new QWidget());
    sidebar->setFixedWidth(200);

    SidebarWidget *sidebarWidget = new SidebarWidget(q);
    sidebar->setWidget(sidebarWidget);
    sidebar->setVisible(false);
}

void MainWindowPrivate::initWidgets()
{
    QStackedWidget *stackedWidget = new QStackedWidget(q);

    //fileselectwidget *startwidget1 = new fileselectwidget(q);
    StartWidget *startwidget = new StartWidget(q);
    ChooseWidget *choosewidget = new ChooseWidget(q);
    PromptWidget *promptidget = new PromptWidget(q);
    ConnectWidget *connectwidget = new ConnectWidget(q);
    FileSelectWidget *connectwidget1 = new FileSelectWidget(qobject_cast<SidebarWidget *>(sidebar->widget()), stackedWidget);
    WaitTransferWidget *waitgwidget = new WaitTransferWidget(q);
    TransferringWidget *transferringwidget = new TransferringWidget(q);
    SuccessWidget *successtranswidget = new SuccessWidget(q);

    //stackedWidget->addWidget(startwidget1);
    stackedWidget->addWidget(startwidget);
    stackedWidget->addWidget(choosewidget);
    stackedWidget->addWidget(promptidget);
    stackedWidget->addWidget(connectwidget);
    stackedWidget->addWidget(connectwidget1);
    stackedWidget->addWidget(waitgwidget);
    stackedWidget->addWidget(transferringwidget);
    stackedWidget->addWidget(successtranswidget);
    stackedWidget->setCurrentIndex(0);

    connect(stackedWidget, &QStackedWidget::currentChanged, this, &MainWindowPrivate::handleCurrentChanged);
    connect(TransferHelper::instance(), &TransferHelper::connectSucceed, this, [transferringwidget, stackedWidget] {
        stackedWidget->setCurrentWidget(transferringwidget);
    });
    connect(TransferHelper::instance(), &TransferHelper::transferSucceed, this, [successtranswidget, stackedWidget] {
        stackedWidget->setCurrentWidget(successtranswidget);
    });

    q->centralWidget()->layout()->addWidget(stackedWidget);
}

void MainWindowPrivate::handleCurrentChanged(int index)
{
}
