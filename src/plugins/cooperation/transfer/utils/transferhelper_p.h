// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERHELPER_P_H
#define TRANSFERHELPER_P_H

#include "gui/transferdialog.h"

#include <co/rpc.h>
#include <co/co.h>

class QDBusInterface;
class FrontendService;
namespace cooperation_transfer {

class TransferHelper;
class TransferHelperPrivate : public QObject
{
    Q_OBJECT
    friend class TransferHelper;

public:
    enum TransferMode {
        SendMode,
        ReceiveMode
    };

    explicit TransferHelperPrivate(TransferHelper *qq);
    ~TransferHelperPrivate();

    void initConfig();

    void localIPCStart();
    bool handlePingBacked();
    void handleSendFiles(const QStringList &fileList);
    void handleApplyTransFiles(int type);
    void handleTryConnect(const QString &ip);
    void handleSetConfig(const QString &key, const QString &value);
    QString handleGetConfig(const QString &key);
    void handleCancelTransfer();

    void transferResult(bool result, const QString &msg);
    void updateProgress(int value, const QString &remainTime);
    uint notifyMessage(uint replacesId, const QString &body,
                       const QStringList &actions, int expireTimeout);

public Q_SLOTS:
    void waitForConfirm(const QString &name);
    void onActionTriggered(uint replacesId, const QString &action);
    void accepted();
    void rejected();

private:
    TransferHelper *q;
    FrontendService *frontendIpcSer { nullptr };
    std::shared_ptr<rpc::Client> rpcClient { nullptr };

    QMap<int, int64_t> fileIds;   // <file_id, last_current_size> 统计正在传输的文件量<文件id，上次已传输量>
    QString sessionId;
    QStringList readyToSendFiles;
    QString sendToWho;

    bool canTransfer { false };
    bool backendOk { false };
    bool thisDestruct { false };

    TransferStatus status { Idle };
    TransferInfo transferInfo;
    TransferDialog *transferDialog { nullptr };
    QDBusInterface *notifyIfc { nullptr };
    uint recvNotifyId { 0 };
    TransferMode currentMode { ReceiveMode };
};

}   // namespace cooperation_transfer

#endif   // TRANSFERHELPER_P_H
