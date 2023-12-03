// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemoncoreplugin.h"
#include "base/baseutils.h"

#include <QDebug>
#include <QUrl>

#include "service/servicemanager.h"
#include "base/baseutils.h"

using namespace daemon_core;

void daemonCorePlugin::initialize()
{
    flag::set_value("rpc_log", "false"); //rpc日志关闭
    flag::set_value("cout", "true");   //终端日志输出
    flag::set_value("journal", "true");   //journal日志

    fastring logdir = deepin_cross::BaseUtils::logDir().toStdString();
    qInfo() << "set logdir: " << logdir.c_str();
    flag::set_value("log_dir", logdir); //日志保存目录
}

bool daemonCorePlugin::start()
{
    ServiceManager *manager = new ServiceManager(this);
    manager->startRemoteServer();
    return true;
}
