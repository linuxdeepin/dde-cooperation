// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "commonutils.h"

#include <QNetworkInterface>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTranslator>
#include <QDir>
#include <QCommandLineParser>

using namespace deepin_cross;

std::string CommonUitls::getFirstIp()
{
    QString ip;
    // QNetworkInterface 类提供了一个主机 IP 地址和网络接口的列表
    foreach (QNetworkInterface netInterface, QNetworkInterface::allInterfaces()) {
        if (!netInterface.flags().testFlag(QNetworkInterface::IsRunning)
            || (netInterface.type() != QNetworkInterface::Ethernet
                && netInterface.type() != QNetworkInterface::Wifi)) {
            // 跳过非运行时, 非有线，非WiFi接口
            continue;
        }

        if (netInterface.name().startsWith("virbr") || netInterface.name().startsWith("vmnet")
            || netInterface.name().startsWith("docker")) {
            // 跳过桥接，虚拟机和docker的网络接口
            qInfo() << "netInterface name:" << netInterface.name();
            continue;
        }

        // 每个网络接口包含 0 个或多个 IP 地址
        QList<QNetworkAddressEntry> entryList = netInterface.addressEntries();
        // 遍历每一个 IP 地址
        foreach (QNetworkAddressEntry entry, entryList) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && entry.ip() != QHostAddress::LocalHost) {
                //IP地址
                ip = QString(entry.ip().toString());
                return ip.toStdString();
            }
        }
    }
    return ip.toStdString();
}

void CommonUitls::loadTranslator()
{
    QStringList translateDirs;
#ifdef _WIN32
    translateDirs << QDir::currentPath() + QDir::separator() + "translations";
#endif

    auto dataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (auto &dir : dataDirs) {
        translateDirs << dir + QDir::separator() + qApp->applicationName() + QDir::separator() + "translations";
    }

    auto locale = QLocale::system();
    QStringList missingQmfiles;
    QStringList translateFilenames { QString("%1_%2").arg(qApp->applicationName()).arg(QLocale::system().name()) };
    const QStringList parseLocalNameList = locale.name().split("_", QString::SkipEmptyParts);
    if (parseLocalNameList.length() > 0)
        translateFilenames << QString("%1_%2").arg(qApp->applicationName()).arg(parseLocalNameList.at(0));

    for (const auto &translateFilename : translateFilenames) {
        for (const auto &dir : translateDirs) {
            QString translatePath = dir + QDir::separator() + translateFilename;
            if (QFile::exists(translatePath + ".qm")) {
                qDebug() << "load translate" << translatePath;
                auto translator = new QTranslator(qApp);
                translator->load(translatePath);
                qApp->installTranslator(translator);
                qApp->setProperty("dapp_locale", locale.name());
                return;
            }
        }

        if (locale.language() != QLocale::English)
            missingQmfiles << translateFilename + ".qm";
    }

    if (missingQmfiles.size() > 0) {
        qWarning() << qApp->applicationName() << "can not find qm files" << missingQmfiles;
    }
}

void CommonUitls::initLog() {
    flag::set_value("rpc_log", "false");   //rpc日志关闭
    flag::set_value("cout", "true");   //终端日志输出
    flag::set_value("journal", "true");   //journal日志
    if (detailLog()) {
        flag::set_value("log_detail", "true"); //详细日志输出
    }

    fastring logdir = logDir().toStdString();
    LOG << "set logdir: " << logdir.c_str();
    flag::set_value("log_dir", logdir);   //日志保存目录
}

QString CommonUitls::logDir()
{
    QString logPath = QString("%1/%2/%3/")
                    .arg(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation))
                    .arg(qApp->organizationName()).arg(qApp->applicationName()); //~/.cache/deepin/xx

    QDir logDir(logPath);
    if (!logDir.exists())
        QDir().mkpath(logPath);

    return logPath;
}

bool CommonUitls::detailLog()
{
    QCommandLineParser parser;
    // 添加自定义选项和参数"-d"
    QCommandLineOption option("d", "Enable detail log");
    parser.addOption(option);

    // 解析命令行参数
    const auto &args = qApp->arguments();
    if (args.size() != 2 || !args.contains("-d"))
        return false;

    parser.process(args);

    // 判断选项是否存在
    bool detailMode = parser.isSet(option);
    return detailMode;
}
