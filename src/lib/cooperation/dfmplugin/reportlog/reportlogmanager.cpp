// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "reportlogmanager.h"
#include "reportlogworker.h"

#include <QThread>
#include <QUrl>
#include <QDebug>

using namespace deepin_cross;

ReportLogManager *ReportLogManager::instance()
{
    qDebug() << "Getting ReportLogManager instance";
    static ReportLogManager ins;
    return &ins;
}

ReportLogManager::ReportLogManager(QObject *parent)
    : QObject (parent)
{
    qDebug() << "ReportLogManager created";
}

ReportLogManager::~ReportLogManager()
{
    if (reportWorkThread) {
        qInfo() << "Log thread start to quit";
        reportWorkThread->quit();
        reportWorkThread->wait(2000);
        qInfo() << "Log thread quited.";
    }
}

void ReportLogManager::init()
{
    qDebug() << "Initializing ReportLogManager";

    reportWorker = new ReportLogWorker();
    qDebug() << "Created ReportLogWorker instance";

    if (!reportWorker->init()) {
        qCritical() << "Failed to initialize ReportLogWorker";
        reportWorker->deleteLater();
        return;
    }

    reportWorkThread = new QThread();
    qDebug() << "Created report worker thread";

    connect(reportWorkThread, &QThread::finished, [&]() {
        qDebug() << "Report worker thread finished, cleaning up";
        reportWorker->deleteLater();
    });
    reportWorker->moveToThread(reportWorkThread);
    qDebug() << "Moved worker to thread";

    initConnection();
    qDebug() << "Initialized signal connections";

    reportWorkThread->start();
    qInfo() << "Report worker thread started";
}

void ReportLogManager::commit(const QString &type, const QVariantMap &args)
{
    qDebug() << "Requesting to commit log, type:" << type << "args:" << args;
    Q_EMIT requestCommitLog(type, args);
    qDebug() << "Log commit request emitted";
}

void ReportLogManager::initConnection()
{
    qDebug() << "Setting up signal connections for ReportLogManager";
    connect(this, &ReportLogManager::requestCommitLog, reportWorker, &ReportLogWorker::commitLog, Qt::QueuedConnection);
    qDebug() << "Connected requestCommitLog signal to worker";
}
