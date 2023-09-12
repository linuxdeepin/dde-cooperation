// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datatransfercoreplugin.h"
#include "base/baseutils.h"

#include <QDebug>
#include <QQmlApplicationEngine>
#include <QUrl>

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
    QUrl url;
    switch (deepin_cross::BaseUtils::osType()) {
    case deepin_cross::BaseUtils::kWindows:
        url = QUrl(QStringLiteral("qrc:/gui/win/mainwin_sender.qml"));
        break;
    case deepin_cross::BaseUtils::kLinux: {
        //url = QUrl(QStringLiteral("qrc:/gui/linux/mainwin_receiver.qml"));
        w = new MainWindow();
        w->show();
        w->moveCenter();
        break;
    }
    default:
        qInfo() << "os type not support, exit" << deepin_cross::BaseUtils::osType();
        return false;
    }

    //    engine = new QQmlApplicationEngine(this);
    //    engine->load(url);

    return true;
}
