// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "inputcooperation.h"
#include "comshare.h"
#include <utils/config.h>
#include <utils/cooconfig.h>

#include <QFile>
#include <QTimer>

#include <utils/utils.h>

InputCooperation::InputCooperation(QObject *parent) : QObject(parent)
{
    _expectedRunning = false;
    _brrierType = BarrierType::Server; // default start as server.

    QSettings *settings = DaemonConfig::instance()->settings();
    _cooConfig = new CooConfig(settings);


//    _cooConfig.setScreenName(m_pLineEditScreenName->text());
//    _cooConfig.setPort(m_pSpinBoxPort->value());
//    _cooConfig.setNetworkInterface(m_pLineEditInterface->text());
//    _cooConfig.setCryptoEnabled(m_pCheckBoxEnableCrypto->isChecked());
//    _cooConfig.setLogLevel(m_pComboLogLevel->currentIndex());
//    _cooConfig.setLogToFile(m_pCheckBoxLogToFile->isChecked());
//    _cooConfig.setLogFilename(m_pLineEditLogFilename->text());
//    _cooConfig.setAutoStart(m_pCheckBoxAutoStart->isChecked());
//    _cooConfig.saveSettings();
}

InputCooperation::~InputCooperation()
{

}

void InputCooperation::setBarrierType(BarrierType type)
{
    _brrierType = type;
}

BarrierType InputCooperation::barrierType() const
{
    return _brrierType;
}

void InputCooperation::restartBarrier()
{
    stopBarrier();
    startBarrier();
}

bool InputCooperation::startBarrier()
{
    LOG << "starting process";
    _expectedRunning = true;

    QString app;
    QStringList args;

    args << "-f" << "--no-tray" << "--debug" << cooConfig().logLevelText();


    args << "--name" << getScreenName();

    setBarrierProcess(new QProcess(this));

#ifndef Q_OS_LINUX

//    if (m_ServerConfig.enableDragAndDrop()) {
//        args << "--enable-drag-drop";
//    }
#endif

//    if (!m_cooConfig->getCryptoEnabled()) {
        args << "--disable-crypto";
//    }

#if defined(Q_OS_WIN)
    // on windows, the profile directory changes depending on the user that
    // launched the process (e.g. when launched with elevation). setting the
    // profile dir on launch ensures it uses the same profile dir is used
    // no matter how its relaunched.
    args << "--profile-dir" << QString::fromStdString("\"" + barrier::DataDirectories::profile().u8string() + "\"");
#endif

    if ((barrierType() == BarrierType::Client && !clientArgs(args, app))
        || (barrierType() == BarrierType::Server && !serverArgs(args, app)))
    {
        stopBarrier();
        return false;
    }


    connect(barrierProcess(), SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(barrierFinished(int, QProcess::ExitStatus)));
    connect(barrierProcess(), SIGNAL(readyReadStandardOutput()), this, SLOT(logOutput()));
    connect(barrierProcess(), SIGNAL(readyReadStandardError()), this, SLOT(logError()));

    LOG << "starting " << QString(barrierType() == BarrierType::Server ? "server" : "client").toStdString();

//    LOG << args.toStdList();

    LOG << QString("command: %1 %2").arg(app, args.join(" ")).toStdString();

    LOG << "config file: "  << this->configFilename().toStdString();
    LOG << "log level: " << cooConfig().logLevelText().toStdString();

    barrierProcess()->start(app, args);
    if (!barrierProcess()->waitForStarted()) {
        ELOG << "Program can not be started: " << app.toStdString();
        return false;
    }

    return true;
}

const char ShutdownCh = 'S';
void InputCooperation::stopBarrier()
{
    LOG << "stopping process";
    _expectedRunning = false;

//    QMutexLocker locker(&m_StopDesktopMutex);
    if (!barrierProcess()) {
        return;
    }

    if (barrierProcess()->isOpen()) {
        // try to shutdown child gracefully
        barrierProcess()->write(&ShutdownCh, 1);
        barrierProcess()->waitForFinished(5000);
        barrierProcess()->close();
    }

    delete barrierProcess();
    setBarrierProcess(nullptr);
}

bool InputCooperation::clientArgs(QStringList& args, QString& app)
{
    app = appPath(cooConfig().barriercName());

    if (!QFile::exists(app))
    {
        WLOG << "Barrier client not found";
        return false;
    }

#if defined(Q_OS_WIN)
    // wrap in quotes so a malicious user can't start \Program.exe as admin.
    app = QString("\"%1\"").arg(app);
#endif

//    if (cooConfig().logToFile())
//    {
//        cooConfig().persistLogDir();
//        args << "--log" << cooConfig().logFilenameCmd();
//    }

    args << "[" + cooConfig().serverIp() + "]:" + QString::number(cooConfig().port());

    return true;
}


bool InputCooperation::serverArgs(QStringList& args, QString& app)
{
    app = appPath(cooConfig().barriersName());

    if (!QFile::exists(app))
    {
        WLOG << "Barrier server not found";
        return false;
    }

#if defined(Q_OS_WIN)
    // wrap in quotes so a malicious user can't start \Program.exe as admin.
    app = QString("\"%1\"").arg(app);
#endif

//    if (cooConfig().logToFile())
//    {
//        cooConfig().persistLogDir();

//        args << "--log" << cooConfig().logFilenameCmd();
//    }

//        args << "--disable-client-cert-checking";

    QString configFilename = this->configFilename();
#if defined(Q_OS_WIN)
    // wrap in quotes in case username contains spaces.
    configFilename = QString("\"%1\"").arg(configFilename);
#endif
    args << "-c" << configFilename << "--address" << address();

    return true;
}

QString InputCooperation::configFilename()
{
    return Util::barrierConfig();
}

QString InputCooperation::getScreenName()
{
    if (cooConfig().screenName() == "") {
        return QHostInfo::localHostName();
    }
    else {
        return cooConfig().screenName();
    }
}

QString InputCooperation::address()
{
    QString address = cooConfig().networkInterface();
    if (!address.isEmpty())
        address = "[" + address + "]";
    return address + ":" + QString::number(cooConfig().port());
}

QString InputCooperation::appPath(const QString& name)
{
    return cooConfig().barrierProgramDir() + name;
}

void InputCooperation::barrierFinished(int exitCode, QProcess::ExitStatus)
{
    if (exitCode == 0) {
        LOG << "process exited normally";
    } else {
        ELOG << "process exited with error code: " << exitCode;
    }

    // auto restart if expect keep running
    if (_expectedRunning) {
        QTimer::singleShot(1000, this, SLOT(startBarrier()));
        LOG << "detected process not running, auto restarting";
    }
}

void InputCooperation::appendLogRaw(const QString& text, bool error)
{
    for (QString line : text.split(QRegExp("\r|\n|\r\n"))) {
        if (!line.isEmpty()) {
            if (error) {
                ELOG << line.toStdString();
            } else {
                LOG << line.toStdString();
            }
        }
    }
}

void InputCooperation::logOutput()
{
    if (_pBarrier)
    {
        appendLogRaw(_pBarrier->readAllStandardOutput(), false);
    }
}

void InputCooperation::logError()
{
    if (_pBarrier)
    {
        appendLogRaw(_pBarrier->readAllStandardError(), true);
    }
}

