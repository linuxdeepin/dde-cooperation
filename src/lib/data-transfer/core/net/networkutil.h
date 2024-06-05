// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKUTIL_H
#define NETWORKUTIL_H

#include <QString>
#include <QObject>
#include <QSharedPointer>

class NetworkUtilPrivate;
class NetworkUtil : public QObject
{
    Q_OBJECT
public:
    static NetworkUtil *instance();

    // setting
    bool doConnect(const QString &ip, const QString &password);
    void disConnect();
    void updatePassword(const QString &code);
    void updateStorageConfig();
    bool sendMessage(const QString &msg);

    //transfer
    void cancelTrans();
    void doSendFiles(const QStringList &fileList);

private:
    explicit NetworkUtil(QObject *parent = nullptr);
    ~NetworkUtil();

private:
    QSharedPointer<NetworkUtilPrivate> d { nullptr };
};

#endif   // NETWORKUTIL_H
