// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatransfercoreplugin.h"
#include "base/baseutils.h"
#include "gui/mainwindow.h"

#include <QDebug>
#include <QUrl>

#include <utils/transferhepler.h>
#include <utils/transferworker.h>

using namespace data_transfer_core;

void DataTransferCorePlugin::initialize()
{
    flag::set_value("rpc_log", "false"); //rpc日志关闭

#ifdef QT_DEBUG
    flag::set_value("cout", "true"); //终端日志输出
#else
    fastring logdir = deepin_cross::BaseUtils::logDir().toStdString();
    qInfo() << "set logdir: " << logdir.c_str();
    flag::set_value("log_dir", logdir); //日志保存目录
#endif
}

bool DataTransferCorePlugin::start()
{
    TransferHelper::instance();
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
