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

    static TransferHelper *instance();

    void init();
    void sendFiles(const QString &ip, const QStringList &fileList);
    TransferStatus transferStatus();

    static void buttonClicked(const QVariantMap &info);
    static bool buttonVisible(const QVariantMap &info);
    static bool buttonClickable(const QVariantMap &info);

protected Q_SLOTS:
    void onConnectStatusChanged(int result, const QString &msg);
    void onTransJobStatusChanged(int id, int result, const QString &msg);
    void onFileTransStatusChanged(const QString &status);
    void onMiscMessage(QString jsonmsg);

private:
    explicit TransferHelper(QObject *parent = nullptr);
    ~TransferHelper();

    void localIPCStart();

    bool handlePingBacked();
    void handleSendFiles(const QStringList &fileList);
    void handleTryConnect(const QString &ip);
    void handleSetConfig(const QString &key, const QString &value);
    QString handleGetConfig(const QString &key);

private:
    FrontendService *frontendIpcSer { nullptr };
    std::shared_ptr<co::pool> coPool { nullptr };

    TransferStatus status { Idle };
    bool backendOk { false };
    bool thisDestruct { false };

    QString sessionId;
    QStringList readyToSendFiles;

    TransferDialog transferDialog;
};

}   // namespace cooperation_transfer

#endif   // TRANSFERHELPER_H
