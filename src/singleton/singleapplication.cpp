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
#include <QDateTime>
#include <QCoreApplication>

#ifdef __linux__
#include <unistd.h>
#endif

using namespace deepin_cross;

SingleApplication::SingleApplication(int &argc, char **argv, int)
    : CrossApplication(argc, argv)
    , localServer(new QLocalServer(this))
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
    // if (qAppName() == "dde-cooperation-daemon") {
        // daemon process should exit after all work is done
        _exit(0); // FIXME: always double free if without this.
    // }
}

void SingleApplication::initConnect()
{
    qDebug() << "Initializing local server connections";
    connect(localServer, &QLocalServer::newConnection, this, &SingleApplication::handleConnection);
    qDebug() << "Local server connections initialized";
}

void SingleApplication::handleConnection()
{
    qDebug() << "new connection is coming: " << qAppName();
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
    if (!nextPendingConnection) {
        qWarning() << "No pending connection available";
        return;
    }

    connect(nextPendingConnection, SIGNAL(readyRead()), this, SLOT(readData()));

    // If data is already available, process it immediately
    if (nextPendingConnection->bytesAvailable() > 0) {
        readData();
    }
}

bool SingleApplication::sendMessage(const QString &key, const QByteArray &message)
{
    qDebug() << "Attempting to send message to:" << key;

    SingleApplication *instance = qobject_cast<SingleApplication *>(QCoreApplication::instance());
    if (!instance) {
        qWarning() << "No SingleApplication instance available";
        return false;
    }

    // Prefer using recorded active socket (avoid redundant testing)
    if (!instance->activeServerName.isEmpty()) {
        qDebug() << "Using recorded active socket:" << instance->activeServerName;
        if (instance->doSendMessage(instance->activeServerName, message)) {
            return true;
        }
        qWarning() << "Failed to send via recorded socket, falling back to socket discovery";
    }

    // Fallback: Find active socket (for cross-process or socket change scenarios)
    QString targetSocket = instance->findActiveSocket(key);
    if (targetSocket.isEmpty()) {
        qWarning() << "No active socket found for message delivery to:" << key;
        return false;
    }

    bool sent = instance->doSendMessage(targetSocket, message);
    if (sent) {
        instance->activeServerName = targetSocket;
    }
    return sent;
}

bool SingleApplication::checkProcess(const QString &key)
{
    QString standardSocket = getSocketName(key, StandardSocket);
    QString backupSocket = getSocketName(key, BackupSocket);

    // Check standard socket
    if (testSocketConnection(standardSocket)) {
        qDebug() << "Found active process on standard socket";
        return true;
    }

    // Check backup socket
    if (testSocketConnection(backupSocket)) {
        qDebug() << "Found active process on backup socket";
        return true;
    }

    qDebug() << "No active process found on either socket";
    return false;
}

bool SingleApplication::setSingleInstance(const QString &key)
{
    QString standardSocket = getSocketName(key, StandardSocket);
    QString backupSocket = getSocketName(key, BackupSocket);

    // Step 1: Check if a process is already running (check both sockets)
    if (checkProcess(key)) {
        qDebug() << "Process already running, will connect to existing instance";
        return false;
    }

    // Step 2: Try to create standard socket
    if (tryCreateSocket(standardSocket)) {
        activeServerName = standardSocket;
        qDebug() << "Successfully set as single instance using standard socket:" << standardSocket;
        return true;
    }

    // Step 3: Standard socket failed, try backup socket
    qDebug() << "Standard socket failed, trying backup socket";
    if (tryCreateSocket(backupSocket)) {
        activeServerName = backupSocket;
        qDebug() << "Successfully set as single instance using backup socket:" << backupSocket;
        return true;
    }

    qWarning() << "Both standard and backup sockets failed for key:" << key;
    return false;
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

        // Clear active socket record
        activeServerName.clear();
        qDebug() << "Local server closed successfully and active socket name cleared";
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

QString SingleApplication::getSocketName(const QString &key, SocketType type)
{
#ifdef linux
    // On Unix systems, use abstract socket namespace (starts with \0)
    // Use a globally accessible name - add a specific prefix to ensure uniqueness
    switch (type) {
    case StandardSocket:
        return QString("@%1-socket").arg(key);
    case BackupSocket:
        return QString("@%1-backup").arg(key);
    }
#else
    // On Windows and other platforms, use the temp path
    switch (type) {
    case StandardSocket:
        return QDir::tempPath() + QDir::separator() + key + ".socket";
    case BackupSocket:
        return QDir::tempPath() + QDir::separator() + key + "-backup.socket";
    }
#endif
    return QString(); // Should never reach here
}

bool SingleApplication::tryCreateSocket(const QString &socketPath)
{
    if (socketPath.isEmpty()) {
        qWarning() << "Empty socket path provided";
        return false;
    }

    // Clean up possible stale socket
    QLocalServer::removeServer(socketPath);
    QFile::remove(socketPath);

    // Set permission options to allow all users access
    localServer->setSocketOptions(QLocalServer::WorldAccessOption);

    if (localServer->listen(socketPath)) {
        qDebug() << "Successfully created socket:" << socketPath;
        return true;
    }

    qDebug() << "Failed to create socket:" << socketPath << "Error:" << localServer->errorString();
    return false;
}

bool SingleApplication::testSocketConnection(const QString &socketPath)
{
    if (socketPath.isEmpty()) {
        return false;
    }

    QLocalSocket testSocket;
    testSocket.connectToServer(socketPath);
    bool connected = testSocket.waitForConnected(1000);

    if (connected) {
        qDebug() << "Successfully connected to:" << socketPath;
        testSocket.close();
        return true;
    }

    qDebug() << "Failed to connect to:" << socketPath;
    return false;
}

QString SingleApplication::findActiveSocket(const QString &key)
{
    QString standardSocket = getSocketName(key, StandardSocket);
    QString backupSocket = getSocketName(key, BackupSocket);

    // Find active socket by priority
    if (testSocketConnection(standardSocket)) {
        qDebug() << "Found active standard socket:" << standardSocket;
        return standardSocket;
    }

    if (testSocketConnection(backupSocket)) {
        qDebug() << "Found active backup socket:" << backupSocket;
        return backupSocket;
    }

    qDebug() << "No active socket found for key:" << key;
    return QString(); // No active socket found
}

bool SingleApplication::doSendMessage(const QString &socketPath, const QByteArray &message)
{
    if (socketPath.isEmpty()) {
        qWarning() << "Empty socket path for message sending";
        return false;
    }

    QLocalSocket *localSocket = new QLocalSocket();

    // Error handling function
    auto errorHandler = [localSocket]() {
        qWarning() << "Socket error:" << localSocket->error() << localSocket->errorString();
    };

    // Connect error signals - Qt5/Qt6 compatibility
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(localSocket, &QLocalSocket::errorOccurred, errorHandler);
#else
    connect(localSocket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), errorHandler);
#endif
    connect(localSocket, &QLocalSocket::disconnected, localSocket, &QLocalSocket::deleteLater);

    localSocket->connectToServer(socketPath);
    if (localSocket->waitForConnected(1000)) {
        if (localSocket->state() == QLocalSocket::ConnectedState && localSocket->isValid()) {
            localSocket->write(message);
            localSocket->flush();
            if (localSocket->waitForBytesWritten(1000)) {
                qDebug() << "Message successfully sent to:" << socketPath;
                localSocket->disconnectFromServer();
                return true;
            }
        }
    }

    qWarning() << "Message send failed to:" << socketPath;
    localSocket->deleteLater();
    return false;
}
