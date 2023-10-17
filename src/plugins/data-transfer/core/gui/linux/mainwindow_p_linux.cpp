#include "../mainwindow.h"
#include "../mainwindow_p.h"

#include <DGuiApplicationHelper>
#include <DTitlebar>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QNetworkConfigurationManager>
#include <QStackedWidget>

#include <gui/connect/choosewidget.h>
#include <gui/connect/connectwidget.h>
#include <gui/connect/networkdisconnectionwidget.h>
#include <gui/connect/promptwidget.h>
#include <gui/connect/searchwidget.h>
#include <gui/connect/startwidget.h>

#include <gui/transfer/successwidget.h>
#include <gui/transfer/transferringwidget.h>
#include <gui/backupload/uploadfilewidget.h>
#include <gui/transfer/waittransferwidget.h>

#include <utils/transferhepler.h>

DWIDGET_USE_NAMESPACE
DTK_USE_NAMESPACE

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
}

void MainWindowPrivate::initWidgets()
{
    QStackedWidget *stackedWidget = new QStackedWidget(q);

    StartWidget *startwidget = new StartWidget(q);
    ChooseWidget *choosewidget = new ChooseWidget(q);
    NetworkDisconnectionWidget *networkdisconnectwidget = new NetworkDisconnectionWidget(q);
    UploadFileWidget *uploadwidget = new UploadFileWidget(q);
    PromptWidget *promptwidget = new PromptWidget(q);
    ConnectWidget *connectwidget = new ConnectWidget(q);
    WaitTransferWidget *waitgwidget = new WaitTransferWidget(q);
    TransferringWidget *transferringwidget = new TransferringWidget(q);
    SuccessWidget *successtranswidget = new SuccessWidget(q);

    stackedWidget->insertWidget(PageName::startwidget, startwidget);
    stackedWidget->insertWidget(PageName::choosewidget, choosewidget);

    stackedWidget->insertWidget(PageName::uploadwidget, uploadwidget);

    stackedWidget->insertWidget(PageName::networkdisconnectwidget, networkdisconnectwidget);
    stackedWidget->insertWidget(PageName::connectwidget, connectwidget);
    stackedWidget->insertWidget(PageName::promptwidget, promptwidget);
    stackedWidget->insertWidget(PageName::waitgwidget, waitgwidget);
    stackedWidget->insertWidget(PageName::transferringwidget, transferringwidget);
    stackedWidget->insertWidget(PageName::successtranswidget, successtranswidget);

    stackedWidget->setCurrentIndex(0);

    connect(stackedWidget, &QStackedWidget::currentChanged, this, &MainWindowPrivate::handleCurrentChanged);
    connect(TransferHelper::instance(), &TransferHelper::connectSucceed, this, [waitgwidget, stackedWidget] {
        stackedWidget->setCurrentWidget(waitgwidget);
    });

    connect(TransferHelper::instance(), &TransferHelper::transferring, this, [transferringwidget, stackedWidget] {
        stackedWidget->setCurrentWidget(transferringwidget);
    });
    connect(TransferHelper::instance(), &TransferHelper::transferSucceed, this, [successtranswidget, stackedWidget] {
        stackedWidget->setCurrentWidget(successtranswidget);
    });

    QNetworkConfigurationManager *configManager = new QNetworkConfigurationManager(this);
    connect(configManager, &QNetworkConfigurationManager::onlineStateChanged, this, [stackedWidget](bool isOnline) {
        int page = stackedWidget->currentIndex();
        // Only these two pages are used networkdisconnectwidget
        if (page != PageName::connectwidget && page != PageName::promptwidget)
            return;
        if (!isOnline)
            stackedWidget->setCurrentIndex(PageName::networkdisconnectwidget);
        else
            stackedWidget->setCurrentIndex(PageName::choosewidget);
    });

    //Adapt Theme Colors
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this,
            [startwidget, choosewidget, uploadwidget, networkdisconnectwidget, connectwidget, promptwidget, waitgwidget, transferringwidget, successtranswidget](DGuiApplicationHelper::ColorType themeType) {
                int theme = themeType == DGuiApplicationHelper::LightType ? 1 : 0;
                startwidget->themeChanged(theme);
                choosewidget->themeChanged(theme);
                uploadwidget->themeChanged(theme);
                networkdisconnectwidget->themeChanged(theme);
                connectwidget->themeChanged(theme);
                promptwidget->themeChanged(theme);
                waitgwidget->themeChanged(theme);
                transferringwidget->themeChanged(theme);
                successtranswidget->themeChanged(theme);
            });

    q->centralWidget()->layout()->addWidget(stackedWidget);
}

void MainWindowPrivate::handleCurrentChanged(int index)
{
}
