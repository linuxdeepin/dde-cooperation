// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatransfercoreplugin.h"
#include "base/baseutils.h"
#include "gui/mainwindow.h"

#include <QDebug>
#include <QUrl>

#include <utils/transferworker.h>

using namespace data_transfer_core;

void DataTransferCorePlugin::initialize()
{
}

bool DataTransferCorePlugin::start()
{
    return loadMainPage();
}

void DataTransferCorePlugin::stop()
{
    if (w) {
        delete w;
        w = nullptr;
    }
}

bool DataTransferCorePlugin::loadMainPage()
{
    w = new MainWindow();
    w->show();
    return true;
}
