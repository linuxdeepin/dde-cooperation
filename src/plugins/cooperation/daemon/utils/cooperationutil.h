// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONUTIL_H
#define COOPERATIONUTIL_H

#include "global_defines.h"

#include <QObject>
#include <QSharedPointer>

namespace daemon_cooperation {

class CooperationUtilPrivate;
class CooperationUtil : public QObject
{
    Q_OBJECT
public:
    static CooperationUtil *instance();

    void registAppInfo(const QString &infoJson);
    void unregistAppInfo();
    void setAppConfig(const QString &key, const QString &value);

    void replyTransRequest(int type);
    void cancelTrans();

private:
    explicit CooperationUtil(QObject *parent = nullptr);
    ~CooperationUtil();

private:
    QSharedPointer<CooperationUtilPrivate> d { nullptr };
};

}   // namespace daemon_cooperation

#endif   // COOPERATIONUTIL_H
