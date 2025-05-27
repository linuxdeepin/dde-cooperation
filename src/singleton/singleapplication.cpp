// SPDX-FileCopyrightText: 2021 - 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "singleapplication.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QWidget>
#include <QLockFile>
#include <QDebug>

using namespace deepin_cross;

SingleApplication::SingleApplication(int &argc, char **argv, int)
    : CrossApplication(argc, argv), localServer(new QLocalServer(this))
{
    qDebug() << "SingleApplication initialized with argc:" << argc;
    setOrganizationName("deepin");
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    initConnect();
}

SingleApplication::~SingleApplication()
{
    qDebug() << "SingleApplication shutting down";
    closeServer();
    qDebug() << "SingleApplication shutdown completed";
}

void SingleApplication::initConnect()
{
    qDebug() << "Initializing local server connections";
    connect(localServer, &QLocalServer::newConnection, this, &SingleApplication::handleConnection);
    qDebug() << "Local server connections initialized";
}

void SingleApplication::handleConnection()
{
    qDebug() << "new connection is coming";
    auto windowList = qApp->topLevelWidgets();
    for (auto w : windowList) {
        if (w->objectName() == "MainWindow") {
            w->show();
            w->raise();
            w->activateWindow();
            break;
        }
    }

    QLocalSocket *nextPendingConnection = localServer->nextPendingConnection();
    connect(nextPendingConnection, SIGNAL(readyRead()), this, SLOT(readData()));
}

bool SingleApplication::sendMessage(const QString &key, const QByteArray &message)
{
    qDebug() << "Attempting to send message to:" << key;
    QLocalSocket *localSocket = new QLocalSocket;
    localSocket->connectToServer(userServerName(key));
    if (localSocket->waitForConnected(1000)) {
        if (localSocket->state() == QLocalSocket::ConnectedState) {
            if (localSocket->isValid()) {
                localSocket->write(message);
                localSocket->flush();
                qDebug() << "Message successfully sent to:" << key;
                return true;
            }
        }
    } else {
        qDebug() << localSocket->errorString();
    }

    qWarning() << "Message send failed for:" << key;
    return false;
}

bool SingleApplication::checkProcess(const QString &key)
{
    // create lock file for check process
    const QString lockFilePath = QDir::tempPath() + QLatin1Char('/') + key + QLatin1String(".lock");
    auto lockFile = new QLockFile(lockFilePath);
    lockFile->setStaleLockTime(0);

    if (!lockFile->tryLock()) {
        // someone else has the lock => process was launched
        qWarning() << key << "has been launched!";
        return true;
    }

    if (key != qAppName()) {
        // release this lock file if not myself, it should be lock by itself.
        lockFile->unlock();
        lockFile->removeStaleLockFile();
    }

    return false;
}

bool SingleApplication::setSingleInstance(const QString &key)
{
    if (checkProcess(key))
        return false;

    QString userKey = userServerName(key);
    if (!localServer->listen(userKey)) {
        // maybe exception crashed, leaving a stale socket; delete it and try again
        QLocalServer::removeServer(userKey);
        if (!localServer->listen(userKey)) {
            qWarning("SingleApplication: unable to make instance listen on %ls: %ls",
                     qUtf16Printable(userKey),
                     qUtf16Printable(localServer->errorString()));

            return false;
        }
    }

    qDebug() << "Successfully set as single instance for:" << key;
    return true;
}

void SingleApplication::readData()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());

    if (!socket)
        return;

    QStringList arguments;
    arguments << this->arguments().first(); // append self app name at first in order to parse commandline.
    for (const QByteArray &arg_base64 : socket->readAll().split(' ')) {
        const QByteArray &arg = QByteArray::fromBase64(arg_base64.simplified());

        if (arg.isEmpty())
            continue;

        arguments << QString::fromLocal8Bit(arg);
    }

    emit SingleApplication::onArrivedCommands(arguments);
}

void SingleApplication::closeServer()
{
    qDebug() << "Closing local server";
    if (localServer) {
        qDebug() << "Removing server:" << localServer->serverName();
        localServer->removeServer(localServer->serverName());
        localServer->close();
        delete localServer;
        localServer = nullptr;
        qDebug() << "Local server closed successfully";
    } else {
        qDebug() << "No local server to close";
    }
}

void SingleApplication::helpActionTriggered()
{
#ifdef linux
    DApplication::handleHelpAction();
#endif
}

void SingleApplication::onDeliverMessage(const QString &app, const QStringList &msg)
{
    qDebug() << "Preparing to deliver message to:" << app;
    QByteArray data { nullptr };
    for (const QString &arg : msg) {
        data.append(arg.toLocal8Bit().toBase64());
        data.append(' ');
    }

    if (!data.isEmpty()) {
        data.chop(1);
        sendMessage(app, data);
    }
}

QString SingleApplication::userServerName(const QString &key)
{
    QString userKey = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation), key);
    if (userKey.isEmpty()) {
        userKey = QString("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::TempLocation), key);
    }
    return userKey;
}
