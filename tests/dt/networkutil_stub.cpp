// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// NetworkUtil 桩实现 — 斩断 data-transfer 真网络依赖。
//
// 真正的 networkutil.cpp 拉 sessionmanager/picojson 真网络栈与 CompatWrapper,
// 在单元测试里既不可控也会触发段错误/阻塞 (见记忆 network-singleton-coverage-ceiling)。
// 这里仅提供 NetworkUtil 所有公开方法的空实现, 让 transferhepler.cpp 等被测源码
// 能链接通过, 同时让运行时不发起任何真实网络/IPC 调用。
//
// 方法签名严格对齐 src/lib/data-transfer/core/net/networkutil.h。

#include "net/networkutil.h"

NetworkUtil::NetworkUtil(QObject *parent)
    : QObject(parent) { }

NetworkUtil::~NetworkUtil() { }

NetworkUtil *NetworkUtil::instance()
{
    static NetworkUtil ins;
    return &ins;
}

bool NetworkUtil::doConnect(const QString &ip, const QString &password)
{
    Q_UNUSED(ip); Q_UNUSED(password);
    return true;
}

void NetworkUtil::disConnect() { }

void NetworkUtil::updatePassword(const QString &code) { Q_UNUSED(code); }

void NetworkUtil::updateStorageConfig() { }

bool NetworkUtil::sendMessage(const QString &msg)
{
    Q_UNUSED(msg);
    return true;
}

void NetworkUtil::cancelTrans(const QString &reason) { Q_UNUSED(reason); }

void NetworkUtil::doSendFiles(const QStringList &fileList) { Q_UNUSED(fileList); }

void NetworkUtil::compatLogin() { }

void NetworkUtil::handleMiscMessage(const QString &msg) { Q_UNUSED(msg); }

#ifdef ENABLE_COMPAT
void NetworkUtil::stop() { }

void NetworkUtil::handleCompatConnectResult(int result, const QString &ip)
{
    Q_UNUSED(result); Q_UNUSED(ip);
}

void NetworkUtil::compatTransJobStatusChanged(int id, int result, const QString &msg)
{
    Q_UNUSED(id); Q_UNUSED(result); Q_UNUSED(msg);
}

void NetworkUtil::compatFileTransStatusChanged(const QString &path, quint64 total, quint64 current, quint64 millisec)
{
    Q_UNUSED(path); Q_UNUSED(total); Q_UNUSED(current); Q_UNUSED(millisec);
}
#endif
