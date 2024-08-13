// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleton/singleapplication.h"
#include "base/baseutils.h"
#include "config.h"
#include "core/datatransfercoreplugin.h"

#include <QDir>
#include <QIcon>
#include <QTranslator>
#include <QDebug>

using namespace data_transfer_core;

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    deepin_cross::SingleApplication app(argc, argv);

#ifdef linux
    app.loadTranslator();
    app.setApplicationName("deepin-data-transfer");
    app.setApplicationDisplayName(app.translate("Application", "UOS data transfer"));
    app.setApplicationVersion(APP_VERSION);
    QIcon icon(":/icon/icon_256.svg");
    app.setProductIcon(icon);
    app.setApplicationAcknowledgementPage("https://www.deepin.org/acknowledgments/" );
    app.setApplicationDescription(app.translate("Application", "UOS transfer tool enables one click migration of your files, personal data, and applications to UOS, helping you seamlessly replace your system."));
#endif


    bool canSetSingle = app.setSingleInstance(app.applicationName());
    if (!canSetSingle) {
        qCritical() << app.applicationName() << "is already running.";
        return 0;
    }

    if (deepin_cross::BaseUtils::isWayland()) {
        // do something
    }
    DataTransferCorePlugin *core = new DataTransferCorePlugin();
    core->start();

    int ret = app.exec();

    core->stop();

    return ret;
}
