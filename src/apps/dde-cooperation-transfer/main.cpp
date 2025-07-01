// SPDX-FileCopyrightText: 2023 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleton/singleapplication.h"
#include "singleton/commandparser.h"
#include "base/baseutils.h"
#include "config.h"

#include "transfer/transferplugin.h"

#include <QDir>
#include <QProcess>

#include <signal.h>

using namespace deepin_cross;
using namespace cooperation_transfer;

const char *dependProc = "dde-cooperation";

static void appExitHandler(int sig)
{
    qInfo() << "break with !SIGTERM! " << sig;
    qApp->quit();
}

int main(int argc, char *argv[])
{
    // qputenv("QT_LOGGING_RULES", "dde-cooperation-transfer.debug=true");
    // qputenv("SLOTIPC_DEBUG", "1");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    deepin_cross::SingleApplication app(argc, argv);
    app.setProperty("onlyTransfer", true);

    bool cooRunning = app.checkProcess(dependProc);
    if (cooRunning) {
        qInfo() << "cooperation App launched!";
    } else {
        qInfo() << "Starting dde-cooperation process in minimized mode";
        // run backend and set minimize
        QProcess::startDetached(dependProc, QStringList() << "-m");
    }

    {
        qInfo() << "Loading translations";
        // 加载翻译
        auto appName = app.applicationName();
        app.setApplicationName(dependProc);
        app.loadTranslator();
        app.setApplicationName(appName);
    }

    bool isSingleInstance = app.setSingleInstance(app.applicationName());
    if (!isSingleInstance) {
        qInfo() << "Another instance is already running";
        QStringList msgs = app.arguments().mid(1); //remove first arg: app name
        if (msgs.isEmpty()) {
            msgs << "top"; // top show
        }
        qWarning() << "new client: " << msgs;
        app.onDeliverMessage(app.applicationName(), msgs);
        return 0;
    } else {
        qInfo() << "Running as single instance";
        CommandParser::instance().process();
        auto sendfiles = CommandParser::instance().processCommand("s");
        if (!sendfiles.isEmpty()) {
            qInfo() << "Found files to send, count:" << sendfiles.size();
            app.setProperty("sendFiles", QVariant::fromValue(sendfiles));
        }
    }

    if (deepin_cross::BaseUtils::isWayland()) {
        qInfo() << "Running under Wayland environment";
        // do something
    }

    qInfo() << "Creating and starting TransferPlugin";
    TransferPlugin *plugin = new TransferPlugin();
    plugin->start();

    QObject::connect(&app, &deepin_cross::SingleApplication::onArrivedCommands, [&] (const QStringList &args) {
        qInfo() << "Received new commands:" << args;
        CommandParser::instance().process(args);
        auto sendfiles = CommandParser::instance().processCommand("s");
        if (!sendfiles.isEmpty()) {
            qInfo() << "Found new files to send from commands, count:" << sendfiles.size();
            app.setProperty("sendFiles", QVariant::fromValue(sendfiles));
        }
    });

    signal(SIGINT, appExitHandler);
    signal(SIGTERM, appExitHandler);
    qInfo() << "Entering application event loop";
    int ret = app.exec();

    qInfo() << "Stopping TransferPlugin";
    plugin->stop();

    qInfo() << "Application exiting with code:" << ret;
    return ret;
}
