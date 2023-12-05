// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHARECOOPERATIONSERVICEMANAGER_H
#define SHARECOOPERATIONSERVICEMANAGER_H

#include <co/fastring.h>
#include "service/comshare.h"
#include "common/commonstruct.h"

#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QTextStream>
#include <QSharedPointer>

class ShareCooperationService;
class ShareCooperationServiceManager : public QObject
{
    Q_OBJECT
public:
    ~ShareCooperationServiceManager() override;
    static ShareCooperationServiceManager *instance();

    QSharedPointer<ShareCooperationService> client();
    QSharedPointer<ShareCooperationService> server();
    void stop();

private:
    explicit ShareCooperationServiceManager(QObject *parent = nullptr);

private:
    QSharedPointer<ShareCooperationService> _client { nullptr };
    QSharedPointer<ShareCooperationService> _server { nullptr };
};

#endif // SHARECOOPERATIONSERVICEMANAGER_H
