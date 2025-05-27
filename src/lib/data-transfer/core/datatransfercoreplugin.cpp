// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatransfercoreplugin.h"
#include "common/commonutils.h"
#include "gui/mainwindow.h"

#include <net/helper/transferhepler.h>

#include <utils/transferutil.h>

using namespace data_transfer_core;
using namespace deepin_cross;

DataTransferCorePlugin::DataTransferCorePlugin(QObject *parent)
    : QObject(parent)
{
    DLOG << "Creating DataTransferCorePlugin instance";
    initialize();
    DLOG << "DataTransferCorePlugin instance created";
}

DataTransferCorePlugin::~DataTransferCorePlugin()
{
    DLOG << "Destroying DataTransferCorePlugin instance";
}

void DataTransferCorePlugin::initialize()
{
    DLOG << "Initializing DataTransferCorePlugin";
    CommonUitls::initLog();
    CommonUitls::loadTranslator();
    DLOG << "DataTransferCorePlugin initialized";
}

bool DataTransferCorePlugin::start()
{
    DLOG << "Starting DataTransferCorePlugin";
    TransferUtil::instance();
    TransferHelper::instance();
    bool result = loadMainPage();
    if (result) {
        LOG << "DataTransferCorePlugin started successfully";
    } else {
        WLOG << "Failed to start DataTransferCorePlugin";
    }
    return result;
}

void DataTransferCorePlugin::stop()
{
    DLOG << "Stopping DataTransferCorePlugin";
    TransferHelper::instance()->finish();
    if (w) {
        DLOG << "Cleaning up main window";
        w->deleteLater();
    }
    LOG << "DataTransferCorePlugin stopped";
}

bool DataTransferCorePlugin::loadMainPage()
{
    DLOG << "Loading main window";
    w = new MainWindow();
    w->show();
    return true;
}
