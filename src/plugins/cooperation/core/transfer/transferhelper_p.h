// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERHELPER_P_H
#define TRANSFERHELPER_P_H

#include "transferhelper.h"
#include "gui/dialogs/transferdialog.h"

#include <co/rpc.h>
#include <co/co.h>

class QDBusInterface;
class FrontendService;
namespace cooperation_core {

class TransferHelper;
class TransferHelperPrivate : public QObject
{
    Q_OBJECT
    friend class TransferHelper;

public:
    struct TransferInfo
    {
        int64_t totalSize = 0;   // 总量
        int64_t transferSize = 0;   // 当前传输量
        int32_t maxTimeSec = 0;   // 耗时
    };

    explicit TransferHelperPrivate(TransferHelper *qq);
    ~TransferHelperPrivate();

    void initConfig();
    QWidget *mainWindow();
    TransferDialog *transDialog();

    void handleSendFiles(const QStringList &fileList);
    void handleApplyTransFiles(int type);
    void handleTryConnect(const QString &ip);
    void handleCancelTransfer();

    void transferResult(bool result, const QString &msg);
    void updateProgress(int value, const QString &remainTime);
    uint notifyMessage(uint replacesId, const QString &body,
                       const QStringList &actions, int expireTimeout);    

private:
    TransferHelper *q;

    QMap<int, int64_t> fileIds;   // <file_id, last_current_size> 统计正在传输的文件量<文件id，上次已传输量>
    QStringList readyToSendFiles;
    QString sendToWho;
    QString targetIp;

    TransferHelper::TransferStatus status { TransferHelper::Idle };
    TransferInfo transferInfo;
    TransferDialog *transferDialog { nullptr };
    QDBusInterface *notifyIfc { nullptr };
    uint recvNotifyId { 0 };
    TransferHelper::TransferMode currentMode { TransferHelper::SendMode };
};

}   // namespace cooperation_core

#endif   // TRANSFERHELPER_P_H
