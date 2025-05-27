// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooconfig.h"
#include "common/constant.h"
#include "common/log.h"

#include <QtCore>
#include <QtNetwork>

#if defined(Q_OS_WIN)
const char CooConfig::m_BarriersName[] = "barriers.exe";
const char CooConfig::m_BarriercName[] = "barrierc.exe";
const char CooConfig::m_ConfigName[] = "barrier.sgc";
#else
const char CooConfig::m_BarriersName[] = "barriers";
const char CooConfig::m_BarriercName[] = "barrierc";
const char CooConfig::m_ConfigName[] = ".barrier.conf"; // default config name under profile()
#endif

static const char *logLevelNames[] = {
    "ERROR",
    "WARNING",
    "NOTE",
    "INFO",
    "DEBUG",
    "DEBUG1",
    "DEBUG2"
};

CooConfig::CooConfig(QSettings *settings)
    : m_pSettings(settings),
      m_ScreenName(),
      m_Port(UNI_SHARE_SERVER_PORT),
      m_TargetServerIp(),
      m_Interface(),
      m_LogLevel(0),
      m_CryptoEnabled(false)
{
    DLOG << "CooConfig constructor";
    Q_ASSERT(m_pSettings);

    loadSettings();
}

CooConfig::~CooConfig()
{
    DLOG << "CooConfig destructor";
    saveSettings();
}

const QString &CooConfig::screenName() const
{
    return m_ScreenName;
}

int CooConfig::port() const
{
    DLOG << "Getting port:" << m_Port;
    return m_Port;
}

QString CooConfig::serverIp() const
{
    DLOG << "Getting server IP:" << m_TargetServerIp.toStdString();
    return m_TargetServerIp;
}

const QString &CooConfig::networkInterface() const
{
    return m_Interface;
}

int CooConfig::logLevel() const
{
    return m_LogLevel;
}

QString CooConfig::barrierProgramDir() const
{
    QString libexec = QString(EXECUTABLE_PROG_DIR);
    if (libexec.isEmpty()) {
        // barrier binaries should be in the same dir.
        return QCoreApplication::applicationDirPath() + "/";
    }
    return libexec + "/";
}

QString CooConfig::logLevelText() const
{
    return logLevelNames[logLevel()];
}

void CooConfig::loadSettings()
{
    DLOG << "Loading settings from config file";
    QString groupname("cooperation_settings");
    settings().beginGroup(groupname);

    m_ScreenName = settings().value("screenName", QHostInfo::localHostName()).toString();
    m_Port = settings().value("port", UNI_SHARE_SERVER_PORT).toInt();
    m_TargetServerIp = settings().value("serverIp").toString();
    m_Interface = settings().value("interface").toString();
    m_LogLevel = settings().value("logLevel", 3).toInt();   // level 3: INFO
    m_CryptoEnabled = settings().value("cryptoEnabled", true).toBool();

    settings().endGroup();
}

void CooConfig::saveSettings()
{
    DLOG << "Saving settings to config file";
    QString groupname("cooperation_settings");
    settings().beginGroup(groupname);

    settings().setValue("screenName", m_ScreenName);
    settings().setValue("port", m_Port);
    settings().setValue("serverIp", m_TargetServerIp);
    settings().setValue("interface", m_Interface);
    settings().setValue("logLevel", m_LogLevel);
    settings().setValue("cryptoEnabled", m_CryptoEnabled);

    settings().endGroup();
    settings().sync();
}

QSettings &CooConfig::settings()
{
    return *m_pSettings;
}

void CooConfig::setScreenName(const QString &s)
{
    DLOG << "Setting screen name:" << s.toStdString();
    m_ScreenName = s;
}

void CooConfig::setPort(int i)
{
    DLOG << "Setting port:" << i;
    m_Port = i;
}

void CooConfig::setServerIp(const QString &s)
{
    DLOG << "Setting server IP:" << s.toStdString();
    m_TargetServerIp = s;
}

void CooConfig::setNetworkInterface(const QString &s)
{
    DLOG << "Setting network interface:" << s.toStdString();
    m_Interface = s;
}

void CooConfig::setLogLevel(int i)
{
    DLOG << "Setting log level:" << i;
    m_LogLevel = i;
}

QString CooConfig::barriersName() const
{
    DLOG << "Getting barriers name:" << m_BarriersName;
    return m_BarriersName;
}

QString CooConfig::barriercName() const
{
    DLOG << "Getting barrierc name:" << m_BarriercName;
    return m_BarriercName;
}

QString CooConfig::configName() const
{
    DLOG << "Getting config name:" << m_ConfigName;
    return m_ConfigName;
}

QString CooConfig::profileDir() const
{
    DLOG << "Getting profile directory:" << m_ProfileDir.toStdString();
    return m_ProfileDir;
}

void CooConfig::setProfileDir(const QString &s)
{
    DLOG << "Setting profile directory:" << s.toStdString();
    m_ProfileDir = s;
}

void CooConfig::setCryptoEnabled(bool e)
{
    DLOG << "Setting crypto enabled:" << e;
    m_CryptoEnabled = e;
}

bool CooConfig::getCryptoEnabled() const
{
    DLOG << "Getting crypto enabled:" << m_CryptoEnabled;
    return m_CryptoEnabled;
}
