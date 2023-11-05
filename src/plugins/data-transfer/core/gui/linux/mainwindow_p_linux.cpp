#include "../mainwindow.h"
#include "../mainwindow_p.h"

#include <DGuiApplicationHelper>
#include <DTitlebar>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>

#include <gui/connect/choosewidget.h>
#include <gui/connect/connectwidget.h>
#include <gui/connect/networkdisconnectionwidget.h>
#include <gui/connect/promptwidget.h>
#include <gui/connect/startwidget.h>

#include <gui/transfer/successwidget.h>
#include <gui/transfer/transferringwidget.h>
#include <gui/backupload/uploadfilewidget.h>
#include <gui/transfer/errorwidget.h>
#include <gui/transfer/resultdisplay.h>
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
    ErrorWidget *errorwidget = new ErrorWidget(q);
    SuccessWidget *successtranswidget = new SuccessWidget(q);
    ResultDisplayWidget *resultwidget = new ResultDisplayWidget(q);

    stackedWidget->insertWidget(PageName::startwidget, startwidget);
    stackedWidget->insertWidget(PageName::choosewidget, choosewidget);

    stackedWidget->insertWidget(PageName::uploadwidget, uploadwidget);

    stackedWidget->insertWidget(PageName::networkdisconnectwidget, networkdisconnectwidget);
    stackedWidget->insertWidget(PageName::promptwidget, promptwidget);
    stackedWidget->insertWidget(PageName::connectwidget, connectwidget);
    stackedWidget->insertWidget(PageName::waitgwidget, waitgwidget);
    stackedWidget->insertWidget(PageName::transferringwidget, transferringwidget);
    stackedWidget->insertWidget(PageName::errorwidget, errorwidget);
    stackedWidget->insertWidget(PageName::successtranswidget, successtranswidget);
    stackedWidget->insertWidget(PageName::resultwidget, resultwidget);

    stackedWidget->setCurrentIndex(0);

    connect(stackedWidget, &QStackedWidget::currentChanged, this, [transferringwidget](int index) {
        if (index == PageName::choosewidget)
            transferringwidget->clear();
    });
    connect(TransferHelper::instance(), &TransferHelper::connectSucceed, this, [stackedWidget] {
        stackedWidget->setCurrentIndex(PageName::waitgwidget);
    });

    connect(TransferHelper::instance(), &TransferHelper::transferring, this, [stackedWidget] {
        stackedWidget->setCurrentIndex(PageName::transferringwidget);
    });
    connect(TransferHelper::instance(), &TransferHelper::transferSucceed, this, [stackedWidget](bool isall) {
        int nextpage = isall ? PageName::successtranswidget : PageName::resultwidget;
        stackedWidget->setCurrentIndex(nextpage);
    });

    connect(TransferHelper::instance(), &TransferHelper::onlineStateChanged,
            [stackedWidget, errorwidget](bool online) {
                if (online)
                    return;
                int index = stackedWidget->currentIndex();
                //only these need jump to networkdisconnectwidget
                if (index == PageName::connectwidget || index == PageName::waitgwidget || index == PageName::promptwidget)
                    stackedWidget->setCurrentIndex(PageName::networkdisconnectwidget);
                if (index == PageName::transferringwidget) {
                    stackedWidget->setCurrentIndex(PageName::errorwidget);
                    errorwidget->setErrorType(ErrorType::networkError);
                }
            });

    connect(TransferHelper::instance(), &TransferHelper::outOfStorage,
            [stackedWidget, errorwidget](int size) {
                stackedWidget->setCurrentIndex(PageName::errorwidget);
                errorwidget->setErrorType(ErrorType::outOfStorageError, size);
            });

    //Adapt Theme Colors
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this,
            [startwidget, errorwidget, resultwidget, choosewidget, uploadwidget, networkdisconnectwidget, connectwidget, promptwidget, waitgwidget, transferringwidget, successtranswidget](DGuiApplicationHelper::ColorType themeType) {
                int theme = themeType == DGuiApplicationHelper::LightType ? 1 : 0;
                errorwidget->themeChanged(theme);
                resultwidget->themeChanged(theme);
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
    emit DGuiApplicationHelper::instance()->themeTypeChanged(DGuiApplicationHelper::instance()->themeType());

    q->centralWidget()->layout()->addWidget(stackedWidget);
}
