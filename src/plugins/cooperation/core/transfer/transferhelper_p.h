// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERHELPER_P_H
#define TRANSFERHELPER_P_H

#include "transferhelper.h"
#include "gui/dialogs/transferdialog.h"

#include <co/rpc.h>
#include <co/co.h>

#include <QTimer>
#include <QMap>

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
        int64_t maxTimeMs = 0;   // 耗时

        void clear()
        {
            totalSize = 0;
            transferSize = 0;
            maxTimeMs = 0;
        }
    };

    explicit TransferHelperPrivate(TransferHelper *qq);
    ~TransferHelperPrivate();

    TransferDialog *transDialog();

    void handleSendFiles(const QStringList &fileList);
    void handleApplyTransFiles(int type);
    void handleTryConnect(const QString &ip);
    void handleCancelTransfer();

    void transferResult(bool result, const QString &msg);
    void updateProgress(int value, const QString &remainTime);

public Q_SLOTS:
    void onVerifyTimeout();

private:
    TransferHelper *q;

    QStringList readyToSendFiles;
    QString sendToWho;

    QAtomicInt status { TransferHelper::Idle };
    TransferInfo transferInfo;
    TransferDialog *transferDialog { nullptr };
    uint recvNotifyId { 0 };

    QTimer confirmTimer;
    bool isTransTimeout = false;
    QString recvFilesSavePath;
};

}   // namespace cooperation_core

#endif   // TRANSFERHELPER_P_H
