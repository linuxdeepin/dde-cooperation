// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "daemoncoreplugin.h"
#include "base/baseutils.h"

#include <QDebug>
#include <QUrl>

#include <service/servicemanager.h>
#include <service/ipc/fsservice.h>

using namespace daemon_core;

void daemonCorePlugin::initialize()
{
}

bool daemonCorePlugin::start()
{
    ServiceManager *manager = new ServiceManager(this);
    manager->startRemoteServer();
    return true;
}
