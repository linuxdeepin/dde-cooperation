// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKUTIL_H
#define NETWORKUTIL_H

#include <QString>
#include <QObject>
#include <QSharedPointer>

namespace cooperation_core {

class NetworkUtilPrivate;
class NetworkUtil : public QObject
{
    Q_OBJECT
public:
    static NetworkUtil *instance();

    // setting
    void updateStorageConfig(const QString &value);
    QString getStorageFolder() const;
    QString getConfirmTargetAddress() const;

    void searchDevice(const QString &ip);
    void pingTarget(const QString &ip);
    void reqTargetInfo(const QString &ip);

    //transfer
    void sendTransApply(const QString &ip);
    void replyTransRequest(bool agree);
    void cancelTrans();
    void doSendFiles(const QStringList &fileList);

    //Keymouse sharing
    void sendShareEvents(const QString &ip);
    void sendDisconnectShareEvents(const QString &ip);
    void replyShareRequest(bool agree);
    void cancelApply(const QString &type);

    QString deviceInfoStr();

private:
    explicit NetworkUtil(QObject *parent = nullptr);
    ~NetworkUtil();

private:
    QSharedPointer<NetworkUtilPrivate> d { nullptr };

    QString _selfFingerPrint { "" };
};

}   // namespace cooperation_core

#endif   // NETWORKUTIL_H
