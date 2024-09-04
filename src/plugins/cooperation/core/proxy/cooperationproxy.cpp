// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "global_defines.h"
#include "cooperationproxy.h"
#include "cooperationdialog.h"
#include "utils/historymanager.h"
#include "utils/cooperationutil.h"
#include "maincontroller/maincontroller.h"

#include "configs/settings/configmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/frontend.h"
#include "ipc/bridge.h"

#include <CuteIPCInterface.h>

#include <QDir>
#include <QUrl>
#include <QTime>
#include <QTimer>
#include <QMessageBox>
#include <QProgressBar>
#include <QProgressDialog>
#include <QDesktopServices>

using TransHistoryInfo = QMap<QString, QString>;
Q_GLOBAL_STATIC(TransHistoryInfo, transHistory)

using namespace deepin_cross;
using namespace cooperation_core;

CooperationProxy::CooperationProxy(QObject *parent)
    : QObject(parent)
{
    transTimer.setInterval(10 * 1000);
    transTimer.setSingleShot(true);
    connect(&transTimer, &QTimer::timeout, this, &CooperationProxy::onConfirmTimeout);

    auto ipc = CooperationUtil::instance()->ipcInterface();
    if (ipc) {
        // bind SIGNAL to SLOT
        ipc->remoteConnect(SIGNAL(cooperationSignal(int, QString)), this, SLOT(secondMessageSlot(int, QString)));
    } else {
        WLOG << "can not connect to: cooperation-daemon";
    }
}

CooperationProxy::~CooperationProxy()
{
}

CooperationProxy *CooperationProxy::instance()
{
    static CooperationProxy ins;
    return &ins;
}

CooperationTransDialog *CooperationProxy::cooperationDialog()
{
    if (!cooperationDlg) {
        cooperationDlg = new CooperationTransDialog(CooperationUtil::instance()->mainWindow());
        connect(cooperationDlg, &CooperationTransDialog::accepted, this, &CooperationProxy::onAccepted);
        connect(cooperationDlg, &CooperationTransDialog::rejected, this, &CooperationProxy::onRejected);
        connect(cooperationDlg, &CooperationTransDialog::canceled, this, &CooperationProxy::onCanceled);
        connect(cooperationDlg, &CooperationTransDialog::completed, this, &CooperationProxy::onCompleted);
        connect(cooperationDlg, &CooperationTransDialog::viewed, this, &CooperationProxy::onViewed);
    }

    return cooperationDlg;
}

void CooperationProxy::waitForConfirm(const QString &name)
{
    isReplied = false;
    transferInfo.clear();
    recvFilesSavePath.clear();
    fromWho = name;

    transTimer.start();
    cooperationDialog()->showConfirmDialog(CommonUitls::elidedText(name, Qt::ElideMiddle, 15));
    cooperationDialog()->show();
}

void CooperationProxy::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    LOG << "id: " << id << " result: " << result << " msg: " << msg.toStdString();
    switch (result) {
    case JOB_TRANS_FAILED:
        if (msg.contains("::not enough")) {
            showTransResult(false, tr("Insufficient storage space, file delivery failed this time. Please clean up disk space and try again!"));
        } else if (msg.contains("::off line")) {
            showTransResult(false, tr("Network not connected, file delivery failed this time. Please connect to the network and try again!"));
        } break;
    case JOB_TRANS_DOING:
        break;
    case JOB_TRANS_FINISHED: {
        // msg: /savePath/deviceName(ip)
        // 获取存储路径和ip
        int startPos = msg.lastIndexOf("(");
        int endPos = msg.lastIndexOf(")");
        if (startPos != -1 && endPos != -1) {
            auto ip = msg.mid(startPos + 1, endPos - startPos - 1);
            recvFilesSavePath = msg;

            transHistory->insert(ip, recvFilesSavePath);
            HistoryManager::instance()->writeIntoTransHistory(ip, recvFilesSavePath);
        }

        showTransResult(true, tr("File sent successfully"));
    } break;
    case JOB_TRANS_CANCELED:
        showTransResult(false, tr("The other party has canceled the file transfer"));
        break;
    default:
        break;
    }
}

void CooperationProxy::onFileTransStatusChanged(const QString &status)
{
    LOG << "file transfer info: " << status.toStdString();
    co::Json statusJson;
    statusJson.parse_from(status.toStdString());
    ipc::FileStatus param;
    param.from_json(statusJson);

    transferInfo.totalSize = param.total;
    transferInfo.transferSize = param.current;
    transferInfo.maxTimeMs = param.millisec;

    // 计算整体进度和预估剩余时间
    double value = static_cast<double>(transferInfo.transferSize) / transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);
    QTime time(0, 0, 0);
    int remain_time;
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        progressValue = 100;
        remain_time = 0;
    } else {
        remain_time = (transferInfo.maxTimeMs * 100 / progressValue - transferInfo.maxTimeMs) / 1000;
    }
    time = time.addSecs(remain_time);

    LOG_IF(FLG_log_detail) << "progressbar: " << progressValue << " remain_time=" << remain_time;
    LOG_IF(FLG_log_detail) << "totalSize: " << transferInfo.totalSize << " transferSize=" << transferInfo.transferSize;

    updateProgress(progressValue, time.toString("hh:mm:ss"));
}

void CooperationProxy::onConfirmTimeout()
{
    if (isReplied)
        return;

    static QString msg(tr("\"%1\" delivery of files to you was interrupted due to a timeout"));
    showTransResult(false, msg.arg(CommonUitls::elidedText(fromWho, Qt::ElideMiddle, 15)));
}

void CooperationProxy::secondMessageSlot(int type, const QString& msg)
{
    co::Json json_obj = json::parse(msg.toStdString());
    if (json_obj.is_null()) {
        WLOG << "parse error from: " << msg.toStdString();
        return;
    }

    switch (type) {
    case IPC_PING: {

    } break;
    case FRONT_TRANS_STATUS_CB: {
        ipc::GenericResult param;
        param.from_json(json_obj);
        QString msg(param.msg.c_str());   // job path

        metaObject()->invokeMethod(this, "onTransJobStatusChanged",
                                    Qt::QueuedConnection,
                                    Q_ARG(int, param.id),
                                    Q_ARG(int, param.result),
                                    Q_ARG(QString, msg));
    } break;
    case FRONT_NOTIFY_FILE_STATUS: {
        metaObject()->invokeMethod(this,
                                    "onFileTransStatusChanged",
                                    Qt::QueuedConnection,
                                    Q_ARG(QString, msg));
    } break;
    case FRONT_APPLY_TRANS_FILE: {
        ApplyTransFiles transferInfo;
        transferInfo.from_json(json_obj);
        LOG << "apply transfer info: " << json_obj;

        switch (transferInfo.type) {
        case ApplyTransType::APPLY_TRANS_APPLY:
            metaObject()->invokeMethod(this,
                                        "waitForConfirm",
                                        Qt::QueuedConnection,
                                        Q_ARG(QString, QString(transferInfo.machineName.c_str())));
            break;
        default:
            break;
        }
    } break;
    case FRONT_SERVER_ONLINE:
        MainController::instance()->regist();
        break;
    default:
        break;
    }
}

void CooperationProxy::replyTransRequest(int type)
{
    isReplied = true;

    // 获取设备名称
    auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
    QString deviceName = value.isValid()
            ? value.toString()
            : QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).dirName();

    // 发送申请文件传输请求
    auto ipc = CooperationUtil::instance()->ipcInterface();
    ipc->call("doApplyTransfer", Q_ARG(QString, qApp->applicationName()), Q_ARG(QString, CooperRegisterName),
              Q_ARG(QString, deviceName), Q_ARG(int, type));
}

void CooperationProxy::onAccepted()
{
    replyTransRequest(ApplyTransType::APPLY_TRANS_CONFIRM);
    cooperationDialog()->hide();
}

void CooperationProxy::onRejected()
{
    replyTransRequest(ApplyTransType::APPLY_TRANS_REFUSED);
    cooperationDialog()->close();
}

void CooperationProxy::onCanceled()
{
    // TRANS_CANCEL 1008
    bool res =  false;
    auto ipc = CooperationUtil::instance()->ipcInterface();
    ipc->call("doOperateJob", Q_RETURN_ARG(bool, res), Q_ARG(int, 1008), Q_ARG(int, 1000), Q_ARG(QString, qApp->applicationName()));
    LOG << "cancelTransferJob result=" << res;

    cooperationDialog()->close();
}

void CooperationProxy::onCompleted()
{
    cooperationDialog()->close();
}

void CooperationProxy::onViewed()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(recvFilesSavePath));
    cooperationDialog()->close();
}

void CooperationProxy::showTransResult(bool success, const QString &msg)
{
    cooperationDialog()->showResultDialog(success, msg);
}

void CooperationProxy::updateProgress(int value, const QString &msg)
{
    static QString title(tr("Receiving files from \"%1\""));
    cooperationDialog()->showProgressDialog(title.arg(CommonUitls::elidedText(fromWho, Qt::ElideMiddle, 15)));
    cooperationDialog()->updateProgressData(value, msg);
}
