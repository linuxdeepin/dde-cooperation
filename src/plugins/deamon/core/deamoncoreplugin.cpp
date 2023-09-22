// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "deamoncoreplugin.h"
#include "base/baseutils.h"

#include <QDebug>
#include <QUrl>

using namespace deamon_core;

void DeamonCorePlugin::initialize()
{
}

bool DeamonCorePlugin::start()
{
    // return loadMainPage();
    return true;
}
