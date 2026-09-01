// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"
#include "base/baseutils.h"
#include "config.h"

#include <dde-cooperation-framework/dpf.h>

#include <QDir>
#include <QIcon>
#include <QThread>
#include <QTranslator>
#include <QDBusConnection>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <fcntl.h>

static constexpr char kPluginInterface[] { "org.deepin.plugin.datatransfer" };
static constexpr char kPluginCore[] { "data-transfer-core" };
#ifdef WIN32
#define LIB_FILE_NAME(lib_name) QString("%1.dll").arg(#lib_name)
#else
#define LIB_FILE_NAME(lib_name) QString("lib%1.so").arg(#lib_name)
#endif

#ifdef linux
// deepin-security-loader 启动协议（与 deepin-home-appstore-client 行为一致）：
// 进程经 loader 附加 deepin-daemon 组启动时，loader 会附加 --fd1/--fd2 命令行参数，
// 分别传入一对管道：子进程把自身 system bus 唯一连接名和需要免鉴权的服务列表
// 以 JSON 写入 fd1，loader 收到后以组身份代为调用各服务的 SetAllowCaller 白名单
// 注册，之后 InstallPackage 等接口免 polkit 鉴权。
// 子进程若不回报，loader 会超时退出并结束子进程（表现为启动闪退），
// 因此经 loader 启动时必须尽早完成回报；直接启动（无 --fd1）则跳过。
static void initSecurityLoaderReport()
{
    const QStringList args = QCoreApplication::arguments();
    int fd1 = -1, fd2 = -1;
    for (int i = 1; i + 1 < args.count(); ++i) {
        if (args.at(i) != QLatin1String("--fd1") && args.at(i) != QLatin1String("--fd2"))
            continue;
        // toInt() 解析失败会返回 0（即 stdin），必须校验，避免误把标准输入当作管道 fd
        bool ok = false;
        const int fd = args.at(i + 1).toInt(&ok);
        if (!ok)
            continue;
        if (args.at(i) == QLatin1String("--fd1"))
            fd1 = fd;
        else
            fd2 = fd;
    }
    if (fd1 < 0)
        return;   // 未经 security-loader 启动，无需回报

    // 管道 fd 保持打开（loader 侧不依赖 EOF 读取，与参照实现 appstore-client 一致），
    // 但设置 FD_CLOEXEC，避免回报管道泄漏给后续 QProcess 派生的子进程
    if (fcntl(fd1, F_SETFD, FD_CLOEXEC) < 0)
        qWarning() << "failed to set FD_CLOEXEC on fd" << fd1;
    if (fd2 >= 0 && fcntl(fd2, F_SETFD, FD_CLOEXEC) < 0)
        qWarning() << "failed to set FD_CLOEXEC on fd" << fd2;

    const QString uniqueName = QDBusConnection::systemBus().baseService();
    if (uniqueName.isEmpty()) {
        qWarning() << "system bus not connected, skip allow-caller report";
        return;
    }

    QJsonObject dest;
    dest.insert(QStringLiteral("DbusName"), QStringLiteral("com.deepin.lastore"));
    dest.insert(QStringLiteral("DbusPath"), QStringLiteral("/com/deepin/lastore"));
    dest.insert(QStringLiteral("DbusInterface"), QStringLiteral("com.deepin.lastore.Manager"));
    QJsonObject msg;
    msg.insert(QStringLiteral("UniqueName"), uniqueName);
    msg.insert(QStringLiteral("DestList"), QJsonArray { dest });

    QFile reportPipe;
    if (!reportPipe.open(fd1, QIODevice::WriteOnly, QFileDevice::DontCloseHandle)) {
        qWarning() << "failed to open security-loader report pipe:" << fd1;
        return;
    }
    reportPipe.write(QJsonDocument(msg).toJson(QJsonDocument::Compact));
    reportPipe.flush();
    reportPipe.close();
    qInfo() << "allow-caller report sent:" << uniqueName;

    // fd2 可读回 {"Result":...,"ErrorMsg":...}。管道为非阻塞模式，loader 处理需要
    // 一次 dbus 往返，轮询等待一小段时间用于日志，失败不影响主流程；
    // 最多等 500ms——该调用位于单例检查之前，不应拖慢启动与窗口激活
    if (fd2 >= 0) {
        QFile resultPipe;
        if (resultPipe.open(fd2, QIODevice::ReadOnly, QFileDevice::DontCloseHandle)) {
            QByteArray resultData;
            for (int i = 0; i < 10 && resultData.isEmpty(); ++i) {
                resultData = resultPipe.readAll();
                if (resultData.isEmpty())
                    QThread::msleep(50);
            }
            resultPipe.close();
            if (resultData.isEmpty()) {
                qWarning() << "no allow-caller result from security-loader within 500ms";
            } else {
                const QJsonObject result = QJsonDocument::fromJson(resultData).object();
                qInfo() << "allow-caller register result:"
                        << result.value(QLatin1String("Result")).toBool()
                        << result.value(QLatin1String("ErrorMsg")).toString();
            }
        }
    }
}
#endif

static bool loadPlugins()
{
    QStringList pluginsDirs;
#ifdef QT_DEBUG
    const QString &pluginsDir { DDE_COOPERATION_PLUGIN_ROOT_DEBUG_DIR };
    qInfo() << QString("Load plugins path : %1").arg(pluginsDir);
    pluginsDirs.push_back(pluginsDir);
    pluginsDirs.push_back(pluginsDir + "/data-transfer");
    pluginsDirs.push_back(pluginsDir + "/data-transfer/core");
#else
    pluginsDirs << QString(DDE_COOPERATION_PLUGIN_ROOT_DIR);
    pluginsDirs << QString(DEEPIN_DATA_TRANS_PLUGIN_DIR);
    pluginsDirs << QDir::currentPath() + "/plugins";
    pluginsDirs << QDir::currentPath() + "/plugins/data-transfer";
    pluginsDirs << QDir::currentPath() + "/plugins/data-transfer/core";
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
    if (!corePlugin->fileName().contains(LIB_FILE_NAME(data-transfer-core)))
        return false;
    if (!DPF_NAMESPACE::LifeCycle::loadPlugin(corePlugin))
        return false;

    // load plugins without core
    if (!DPF_NAMESPACE::LifeCycle::loadPlugins())
        return false;

    return true;
}

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    deepin_cross::SingleApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef linux
    app.loadTranslator();
    app.setApplicationName("deepin-data-transfer");
    app.setApplicationDisplayName(app.translate("Application", "UOS data transfer"));
    app.setApplicationVersion(APP_VERSION);
    QIcon icon(":/icons/icon_256.svg");
    app.setProductIcon(icon);
    app.setApplicationAcknowledgementPage("https://www.deepin.org/acknowledgments/" );
    app.setApplicationDescription(app.translate("Application", "UOS transfer tool enables one click migration of your files, personal data, and applications to UOS, helping you seamlessly replace your system."));
#endif


#ifdef linux
    // 必须先于任何可能退出的分支（含单例失败）：经 security-loader 启动时若不回报
    // 连接名，loader 会超时并结束子进程，表现为启动闪退；该回报同时完成 lastore
    // 免鉴权白名单注册，需早于插件里 SettingHelper 的第一次 InstallPackage
    initSecurityLoaderReport();
#endif

    bool canSetSingle = app.setSingleInstance(app.applicationName());
    if (!canSetSingle) {
        qInfo() << "single application is already running.";
        return 0;
    }

    if (deepin_cross::BaseUtils::isWayland()) {
        // do something
    }

    if (!loadPlugins()) {
        qCritical() << "load plugin failed";
        return -1;
    }

    int ret = app.exec();

    app.closeServer();

#ifdef WIN32
    // FIXME: windows上使用socket，即使线程资源全释放，进程也无法正常退出
    abort();
#endif
    return ret;
}
