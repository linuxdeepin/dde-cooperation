// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "global_defines.h"
#include "cooperationproxy.h"
#include "cooperationdialog.h"
#include "utils/historymanager.h"
#include "utils/cooperationutil.h"

#include "configs/settings/configmanager.h"
#include "common/constant.h"
#include "ipc/frontendservice.h"
#include "ipc/proto/comstruct.h"
#include "ipc/proto/backend.h"

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

using namespace cooperation_core;

CooperationProxy::CooperationProxy(QObject *parent)
    : QObject(parent)
{
    localIPCStart();

    UNIGO([this] {
        backendOk = pingBackend();
        LOG << "The result of ping backend is " << backendOk;
    });
}

CooperationProxy::~CooperationProxy()
{
    thisDestruct = true;
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
    isTransTimeout = false;
    transferInfo.clear();
    recvFilesSavePath.clear();
    fileIds.clear();
    fromWho = name;

    // 超时处理
    QTimer::singleShot(10 * 1000, this, [this] { isTransTimeout = true; });
    cooperationDialog()->showConfirmDialog(name);
    cooperationDialog()->show();
}

void CooperationProxy::onTransJobStatusChanged(int id, int result, const QString &msg)
{
    LOG << "id: " << id << " result: " << result << " msg: " << msg.toStdString();
    switch (result) {
    case JOB_TRANS_FAILED:
        break;
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

    if (fileIds.contains(param.file_id)) {
        // 已经记录过，只更新数据
        int64_t increment = param.current - fileIds[param.file_id];
        transferInfo.transferSize += increment;   //增量值
        fileIds[param.file_id] = param.current;

        if (param.current >= param.total) {
            // 此文件已完成，从文件统计中删除
            fileIds.remove(param.file_id);
        }
    } else {
        transferInfo.totalSize += param.total;
        transferInfo.transferSize += param.current;
        fileIds.insert(param.file_id, param.current);
    }

    if (param.second > transferInfo.maxTimeSec)
        transferInfo.maxTimeSec = param.second;

    // 计算传输进度与剩余时间
    double value = static_cast<double>(transferInfo.transferSize) / transferInfo.totalSize;
    int progressValue = static_cast<int>(value * 100);
    if (progressValue <= 0) {
        return;
    } else if (progressValue >= 100) {
        updateProgress(100, "00:00:00");
    } else {
        int remainTime = transferInfo.maxTimeSec * 100 / progressValue - transferInfo.maxTimeSec;
        QTime time(0, 0, 0);
        time = time.addSecs(remainTime);
        updateProgress(progressValue, time.toString("hh:mm:ss"));
    }
}

bool CooperationProxy::pingBackend()
{
    rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_PORT, false);
    co::Json req, res;

    ipc::PingBackParam backParam;
    backParam.who = CooperRegisterName;
    backParam.version = fastring(UNI_IPC_PROTO);
    backParam.cb_port = UNI_IPC_BACKEND_COOPER_PLUGIN_PORT;

    req = backParam.as_json();
    req.add_member("api", "Backend.ping");   //BackendImpl::ping

    rpcClient.call(req, res);
    rpcClient.close();
    sessionId = res.get("msg").as_string().c_str();   // save the return session.

    //CallResult
    return res.get("result").as_bool() && !sessionId.isEmpty();
}

void CooperationProxy::localIPCStart()
{
    if (frontendIpcSer) return;

    frontendIpcSer = new FrontendService(this);

    UNIGO([this]() {
        while (!thisDestruct) {
            BridgeJsonData bridge;
            frontendIpcSer->bridgeChan()->operator>>(bridge);   //300ms超时
            if (!frontendIpcSer->bridgeChan()->done()) {
                // timeout, next read
                continue;
            }

            co::Json json_obj = json::parse(bridge.json);
            if (json_obj.is_null()) {
                WLOG << "parse error from: " << bridge.json.c_str();
                continue;
            }

            switch (bridge.type) {
            case IPC_PING: {
                ipc::PingFrontParam param;
                param.from_json(json_obj);

                bool result = false;
                fastring my_ver(FRONTEND_PROTO_VERSION);
                // test ping 服务测试用

                if (my_ver.compare(param.version) == 0 && (param.session.compare(sessionId.toStdString()) == 0 || param.session.compare("backendServerOnline") == 0)) {
                    result = true;
                } else {
                    WLOG << param.version.c_str() << " =version not match= " << my_ver.c_str();
                }

                BridgeJsonData res;
                res.type = IPC_PING;
                res.json = result ? param.session : "";   // 成功则返回session，否则为空

                frontendIpcSer->bridgeResult()->operator<<(res);
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
                QString objstr(bridge.json.c_str());
                metaObject()->invokeMethod(this,
                                           "onFileTransStatusChanged",
                                           Qt::QueuedConnection,
                                           Q_ARG(QString, objstr));
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
                pingBackend();
                break;
            default:
                break;
            }
        }
    });

    // start ipc services
    ipc::FrontendImpl *frontendimp = new ipc::FrontendImpl();
    frontendimp->setInterface(frontendIpcSer);

    rpc::Server().add_service(frontendimp).start("0.0.0.0", UNI_IPC_BACKEND_COOPER_PLUGIN_PORT, "/frontend", "", "");
}

void CooperationProxy::replyTransRequest(int type)
{
    UNIGO([=] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
        co::Json res;
        // 获取设备名称
        auto value = ConfigManager::instance()->appAttribute("GenericAttribute", "DeviceName");
        QString deviceName = value.isValid()
                ? value.toString()
                : QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).dirName();

        ApplyTransFiles transInfo;
        transInfo.appname = CooperRegisterName;
        transInfo.type = type;
        transInfo.machineName = deviceName.toStdString();

        co::Json req = transInfo.as_json();
        req.add_member("api", "Backend.applyTransFiles");
        rpcClient.call(req, res);
        rpcClient.close();
    });
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
    UNIGO([this] {
        rpc::Client rpcClient("127.0.0.1", UNI_IPC_BACKEND_COOPER_TRAN_PORT, false);
        co::Json req, res;

        ipc::TransJobParam jobParam;
        jobParam.session = sessionId.toStdString();
        jobParam.job_id = 1000;
        jobParam.appname = CooperRegisterName;

        req = jobParam.as_json();
        req.add_member("api", "Backend.cancelTransJob");   //BackendImpl::cancelTransJob
        rpcClient.call(req, res);
        rpcClient.close();
        LOG << "cancelTransferJob" << res.get("result").as_bool() << res.get("msg").as_string().c_str();
    });

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
    cooperationDialog()->showProgressDialog(title.arg(fromWho));
    cooperationDialog()->updateProgressData(value, msg);
}
