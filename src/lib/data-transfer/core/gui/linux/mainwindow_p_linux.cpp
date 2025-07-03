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

#include <gui/transfer/transferringwidget.h>
#include <gui/backupload/uploadfilewidget.h>
#include <gui/transfer/errorwidget.h>
#include <gui/transfer/resultdisplay.h>
#include <gui/transfer/waittransferwidget.h>

#include <net/helper/transferhepler.h>

DWIDGET_USE_NAMESPACE
DTK_USE_NAMESPACE

using namespace data_transfer_core;

void MainWindowPrivate::initWindow()
{
    DLOG << "Initializing main window";

    q->setWindowTitle(tr("UOS data transfer"));
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
    DLOG << "Main window initialized";
}

void MainWindowPrivate::initWidgets()
{
    DLOG << "Initializing widgets and pages";

    stackedWidget = new QStackedWidget(q);

    StartWidget *startwidget = new StartWidget(q);
    ChooseWidget *choosewidget = new ChooseWidget(q);
    NetworkDisconnectionWidget *networkdisconnectwidget = new NetworkDisconnectionWidget(q);
    UploadFileWidget *uploadwidget = new UploadFileWidget(q);
    PromptWidget *promptwidget = new PromptWidget(q);
    ConnectWidget *connectwidget = new ConnectWidget(q);
    WaitTransferWidget *waitgwidget = new WaitTransferWidget(q);
    TransferringWidget *transferringwidget = new TransferringWidget(q);
    ErrorWidget *errorwidget = new ErrorWidget(q);
    ResultDisplayWidget *resultwidget = new ResultDisplayWidget(q);

    stackedWidget->insertWidget(PageName::startwidget, startwidget);
    stackedWidget->insertWidget(PageName::choosewidget, choosewidget);

    stackedWidget->insertWidget(PageName::uploadwidget, uploadwidget);

    stackedWidget->insertWidget(PageName::networkdisconnectwidget, networkdisconnectwidget);
    stackedWidget->insertWidget(PageName::promptwidget, promptwidget);
    stackedWidget->insertWidget(PageName::connectwidget, connectwidget);
    stackedWidget->insertWidget(PageName::waitwidget, waitgwidget);
    stackedWidget->insertWidget(PageName::transferringwidget, transferringwidget);
    stackedWidget->insertWidget(PageName::errorwidget, errorwidget);
    stackedWidget->insertWidget(PageName::resultwidget, resultwidget);

    stackedWidget->setCurrentIndex(PageName::startwidget);

    connect(TransferHelper::instance(), &TransferHelper::clearWidget, this, [transferringwidget, resultwidget, uploadwidget]() {
        DLOG << "Clearing widgets content";
        transferringwidget->clear();
        resultwidget->clear();
        uploadwidget->clear();
    });
    connect(TransferHelper::instance(), &TransferHelper::connectSucceed, this, [this] {
        DLOG << "Connection succeeded, switching to wait widget";
        stackedWidget->setCurrentIndex(PageName::waitwidget);
    });

    connect(TransferHelper::instance(), &TransferHelper::transferring, this, [this] {
        DLOG << "Transfer started, switching to transferring widget";
        stackedWidget->setCurrentIndex(PageName::transferringwidget);
    });
    connect(TransferHelper::instance(), &TransferHelper::transferFinished, this, [this] {
        DLOG << "Transfer finished, switching to result widget";
        stackedWidget->setCurrentIndex(PageName::resultwidget);
    });

    connect(TransferHelper::instance(), &TransferHelper::onlineStateChanged,
            [this, errorwidget](bool online) {
                DLOG << "Online state changed:" << online;
                if (online) {
                    DLOG << "Network is online, returning";
                    return;
                }
                int index = stackedWidget->currentIndex();
                //only these need jump to networkdisconnectwidget
                if (index == PageName::connectwidget || index == PageName::waitwidget || index == PageName::promptwidget) {
                    DLOG << "Current page is connect, wait, or prompt widget, switching to networkdisconnectwidget";
                    stackedWidget->setCurrentIndex(PageName::networkdisconnectwidget);
                }
                if (index == PageName::transferringwidget) {
                    WLOG << "receiver > network offline, jump to errorwidget";
                    DLOG << "Current page is transferring widget, switching to errorwidget due to network offline";
                    stackedWidget->setCurrentIndex(PageName::errorwidget);
                    errorwidget->setErrorType(ErrorType::networkError);
                }
            });

    //disconect transfer
    connect(TransferHelper::instance(), &TransferHelper::disconnected,
            [this, errorwidget]() {
                DLOG << "Disconnected signal received";
                int index = stackedWidget->currentIndex();
                if (index == PageName::errorwidget) {
                    DLOG << "Current page is errorwidget, returning";
                    return;
                }

                if (index == PageName::transferringwidget || index == PageName::waitwidget) {
                    WLOG << "receiver > disconnected, jump to errorwidget";
                    DLOG << "Current page is transferring or wait widget, switching to errorwidget due to disconnection";
                    stackedWidget->setCurrentIndex(PageName::errorwidget);
                    errorwidget->setErrorType(ErrorType::networkError);
                }
            });

    connect(TransferHelper::instance(), &TransferHelper::outOfStorage,
            [this, errorwidget](int size) {
                WLOG << "receiver > out of storage, jump to errorwidget";
                stackedWidget->setCurrentIndex(PageName::errorwidget);
                errorwidget->setErrorType(ErrorType::outOfStorageError, size);
            });

    //Adapt Theme Colors
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this,
            [startwidget, errorwidget, resultwidget, choosewidget, uploadwidget, networkdisconnectwidget, connectwidget, promptwidget, waitgwidget, transferringwidget](DGuiApplicationHelper::ColorType themeType) {
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
            });
    emit DGuiApplicationHelper::instance()->themeTypeChanged(DGuiApplicationHelper::instance()->themeType());

    connect(TransferHelper::instance(), &TransferHelper::changeWidget, [this](PageName index) {
        DLOG << "Changing to widget:" << index;
        stackedWidget->setCurrentIndex(index);
    });
    q->centralWidget()->layout()->addWidget(stackedWidget);
    DLOG << "Main window widgets initialized";
}
