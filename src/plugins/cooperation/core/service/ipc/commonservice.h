// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMMON_SERVICE_H
#define COMMON_SERVICE_H


#include <QObject>

#include "rpc/common.h"

namespace ipc {

class CommonImpl : public Common {
  public:
    CommonImpl() = default;
    virtual ~CommonImpl() = default;

    virtual void compatible(co::Json& req, co::Json& res) override;

    virtual void syncConfig(co::Json& req, co::Json& res) override;

    virtual void syncPeers(co::Json& req, co::Json& res) override;

    virtual void tryConnect(co::Json& req, co::Json& res) override;

    virtual void tryTargetSpace(co::Json& req, co::Json& res) override;

    virtual void tryApplist(co::Json& req, co::Json& res) override;

    virtual void chatMessage(co::Json& req, co::Json& res) override;

    virtual void miscMessage(co::Json& req, co::Json& res) override;

    virtual void commNotify(co::Json& req, co::Json& res) override;
};

} // ipc

class CommonService : public QObject
{
    Q_OBJECT
public:
    explicit CommonService(QObject *parent = nullptr);
    ~CommonService();

signals:

public slots:

private:

};

#endif // COMMON_SERVICE_H