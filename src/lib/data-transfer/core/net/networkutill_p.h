// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKUTIL_P_H
#define NETWORKUTIL_P_H

#include <QObject>

class SessionManager;
class NetworkUtil;
class NetworkUtilPrivate : public QObject
{
    friend class NetworkUtil;
    Q_OBJECT
public:
    explicit NetworkUtilPrivate(NetworkUtil *qq);
    ~NetworkUtilPrivate();

public Q_SLOTS:
    void handleConnectStatus(int result, QString reason);
    void handleTransChanged(int status, const QString &path, quint64 size);

private:
    NetworkUtil *q { nullptr };
    SessionManager *sessionManager { nullptr };

    QString confirmTargetAddress {};   // remote ip address
    QString storageFolder = {};   //sub folder under storage dir config
};

#endif   // NETWORKUTIL_P_H
