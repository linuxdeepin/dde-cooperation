// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base/baseutils.h"
#include "config.h"

#include <dde-cooperation-framework/dpf.h>
#include <QDir>
#include <QProcess>
#include <QTimer>

#define BASEPROTO_PORT 51597

static constexpr char kPluginInterface[] { "org.deepin.plugin.daemon" };
static constexpr char kPluginCore[] { "daemon-core" };
static constexpr char kfallbackFile[] { "/tmp/cooperation-fallback" };

static bool loadPlugins()
{
    QStringList pluginsDirs;
#ifdef QT_DEBUG
    const QString &pluginsDir { DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR };
    qInfo() << QString("Load plugins path : %1").arg(pluginsDir);
    pluginsDirs.push_back(pluginsDir);
    pluginsDirs.push_back(pluginsDir + "/daemon");
    pluginsDirs.push_back(pluginsDir + "/daemon/core");
#else
    pluginsDirs << QString(DDE_COOPERATION_PLUGIN_ROOT_DIR);
    pluginsDirs << QString(DEEPIN_DAEMON_PLUGIN_DIR);
    pluginsDirs << QDir::currentPath() + "/plugins";
    pluginsDirs << QDir::currentPath() + "/plugins/daemon";
    pluginsDirs << QDir::currentPath() + "/plugins/daemon/core";
#endif
#if defined(WIN32)
    pluginsDirs << QCoreApplication::applicationDirPath();
#endif

    qInfo() << "Using plugins dir:" << pluginsDirs;
    // TODO(zhangs): use config
    static const QStringList kLazyLoadPluginNames {};
    QStringList blackNames;

    DPF_NAMESPACE::LifeCycle::initialize({ kPluginInterface }, pluginsDirs, blackNames, kLazyLoadPluginNames);

    qInfo() << "Depend library paths:" << QCoreApplication::libraryPaths();
    qInfo() << "Load plugin paths: " << dpf::LifeCycle::pluginPaths();

    // read all plugins in setting paths
    if (!DPF_NAMESPACE::LifeCycle::readPlugins())
        return false;

    // We should make sure that the core plugin is loaded first
    auto corePlugin = DPF_NAMESPACE::LifeCycle::pluginMetaObj(kPluginCore);
    if (corePlugin.isNull())
        return false;
    if (!corePlugin->fileName().contains(kPluginCore))
        return false;
    if (!DPF_NAMESPACE::LifeCycle::loadPlugin(corePlugin))
        return false;

    // load plugins without core
    if (!DPF_NAMESPACE::LifeCycle::loadPlugins())
        return false;

    return true;
}

bool isActiveUser()
{
#ifdef _WIN32
    return "admin";
#endif
    QString username = "";
    // 执行 loginctl user-status 命令
    QProcess process;
    process.start("loginctl list-sessions");
    process.waitForFinished(3000);

    // 获取命令输出
    QString output = process.readAllStandardOutput();
    if (output.isEmpty()) {
        qCritical() << "loginctl list-sessions empty out!";
        return true;
    }
    qCritical() << output;
    QMap<QString, QString> sessions;
    auto infoList = output.split("\n");
    if (infoList.length() < 2) {
        qCritical() << "loginctl list-sessions empty session!";
        return true;
    }
    auto first = infoList.takeFirst();
    auto index = first.indexOf("TTY");
    if (index < 0) {
        qCritical() << "loginctl list-sessions empty TTY string!";
        return true;
    }
    QRegExp reg(" +");
    for(const auto &line : infoList) {
        if (line.isEmpty())
            break;
        if (line.length() <= index || !line.mid(index).replace(" ", "").isEmpty())
            continue;
        auto lineInfo = line.trimmed().split(reg);
        if (lineInfo.length() < 3)
            continue;

        sessions.insert(lineInfo.at(0), lineInfo.at(2));
    }

    foreach (auto session, sessions.keys()) {
        process.start("loginctl session-status " + session);
        process.waitForFinished(-1);

        // 获取命令输出
        QString output = process.readAllStandardOutput();
        if (output.isEmpty()) {
            qCritical() << "do not get session-status:" << session;
            continue;
        }

        // 解析输出，检查状态和桌面状态
        bool isActive = false;
        bool isDesktopActive = false;

        QStringList lines = output.split('\n');
        for (const QString& line : lines) {
            if (line.contains("Desktop:")) {
                isDesktopActive = true;
            }
            if (line.contains("State: active")) {
                isActive = true;
            }
        }

        // 判断用户状态和桌面状态
        if (isActive && isDesktopActive) {
            username = sessions.take(session);
        }
    }

    QString curUser = QDir::home().dirName();
    qCritical() << "active session user:" << username << " current user:" << curUser;

    return (curUser.compare(username) == 0);
}

bool portInUse(int port)
{
    QProcess process;
    process.start("netstat -ano");
    process.waitForFinished(3000);

    // 获取命令输出
    QString output = process.readAllStandardOutput();
    if (output.contains("0.0.0.0:" + QString::number(port)))
        return true;

    return false;
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QCoreApplication app(argc, argv);
    app.setOrganizationName("deepin");

    if (deepin_cross::BaseUtils::isWayland()) {
        // do something
    }

#ifdef __linux__
    // 只运行在登录会话用户
    if (!isActiveUser()) {
        qCritical() << "exit, inactive desktop session.";
        return 1;
    }

    QString filePath = kfallbackFile;
    QFile file(filePath);
    bool inUse = portInUse(BASEPROTO_PORT);
    if (inUse) {
        qCritical() << "exit, network port (" << BASEPROTO_PORT << ") is busing........";
        if (!file.exists()) {
            // 创建监视文件
            QString data = "fallback!";
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << data;
                file.close();
            }
        }
        return 1;
    } else {
        if (file.exists()) {
            file.remove();
        }
    }

    QTimer timer;
    timer.setInterval(3000);
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        QFile file(filePath);
        if (file.exists()) {
            file.remove();
            QProcess process;
            process.start("systemctl --user restart cooperation-daemon.service");
            process.waitForFinished(-1);
            qInfo() << "service restarted now!!";
        }
    });
    timer.start();

#endif

    if (!loadPlugins()) {
        qCritical() << "load plugin failed";
        return -1;
    }

    int ret = app.exec();

    return ret;
}
