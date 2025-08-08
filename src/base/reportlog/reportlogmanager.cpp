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
    static ReportLogManager ins;
    return &ins;
}

ReportLogManager::ReportLogManager(QObject *parent)
    : QObject (parent)
{
    qInfo() << "ReportLogManager instance created";
}

ReportLogManager::~ReportLogManager()
{
    qInfo() << "ReportLogManager instance destroyed";
    if (reportWorkThread) {
        qInfo() << "Log thread start to quit";
        reportWorkThread->quit();
        reportWorkThread->wait(2000);
        qInfo() << "Log thread quited.";
    }
}

void ReportLogManager::init()
{
    qInfo() << "Initializing ReportLogManager";
    reportWorker = new ReportLogWorker();
    if (!reportWorker->init()) {
        qInfo() << "Failed to initialize ReportLogWorker";
        reportWorker->deleteLater();
        return;
    }

    reportWorkThread = new QThread();
    connect(reportWorkThread, &QThread::finished, [&]() {
        qInfo() << "ReportWorkThread finished";
        reportWorker->deleteLater();
    });
    reportWorker->moveToThread(reportWorkThread);

    initConnection();

    qInfo() << "Starting ReportWorkThread";
    reportWorkThread->start();
    qInfo() << "ReportLogManager initialized successfully";
}

void ReportLogManager::commit(const QString &type, const QVariantMap &args)
{
    qInfo() << "Committing log of type:" << type << "with args:" << args;
    Q_EMIT requestCommitLog(type, args);
}

void ReportLogManager::initConnection()
{
    qInfo() << "Initializing connection between ReportLogManager and ReportLogWorker";
    connect(this, &ReportLogManager::requestCommitLog, reportWorker, &ReportLogWorker::commitLog, Qt::QueuedConnection);
}
