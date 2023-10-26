// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include "gui/transferdialog.h"

#include <co/rpc.h>
#include <co/co.h>

#include <QVariantMap>

class FrontendService;

namespace cooperation_transfer {

class TransferHelper : public QObject
{
    Q_OBJECT

public:
    enum TransferStatus {
        Idle,
        Connecting,
        Transfering
    };

    struct TransferInfo
    {
        int64_t totalSize;   // 总量
        int64_t transferSize;   // 当前传输量
        int32_t maxTimeSec;   // 耗时
    };

    static TransferHelper *instance();

    void init();
    void sendFiles(const QString &ip, const QString &devName, const QStringList &fileList);
    TransferStatus transferStatus();

    static void buttonClicked(const QVariantMap &info);
    static bool buttonVisible(const QVariantMap &info);
    static bool buttonClickable(const QVariantMap &info);

protected Q_SLOTS:
    void onConnectStatusChanged(int result, const QString &msg);
    void onTransJobStatusChanged(int id, int result, const QString &msg);
    void onFileTransStatusChanged(const QString &status);
    void onMiscMessage(QString jsonmsg);
    void cancelTransfer();

private:
    explicit TransferHelper(QObject *parent = nullptr);
    ~TransferHelper();

    void localIPCStart();

    bool handlePingBacked();
    void handleSendFiles(const QStringList &fileList);
    void handleTryConnect(const QString &ip);
    void handleSetConfig(const QString &key, const QString &value);
    QString handleGetConfig(const QString &key);
    void handleCancelTransfer();
    void registerMe(bool unreg, const QString &info);

private:
    FrontendService *frontendIpcSer { nullptr };
    std::shared_ptr<co::pool> coPool { nullptr };

    TransferStatus status { Idle };
    bool backendOk { false };
    bool thisDestruct { false };

    TransferInfo transferInfo;
    QMap<int, QString> jobMap;   // <jobId, filePath>
    QMap<int, int64_t> fileIds;   // <file_id, last_current_size> 统计正在传输的文件量<文件id，上次已传输量>
    QString sessionId;
    QStringList readyToSendFiles;
    QString sendToWho;

    TransferDialog transferDialog;
    bool canTransfer { false };
};

}   // namespace cooperation_transfer

#endif   // TRANSFERHELPER_H
