// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "reportlogworker.h"
#include "datas/cooperationreportdata.h"
#include "datas/reportdatainterface.h"

#include <QDebug>
#include <QApplication>
#include <QJsonDocument>

using namespace deepin_cross;

ReportLogWorker::ReportLogWorker(QObject *parent)
    : QObject(parent)
{
    qDebug() << "ReportLogWorker created";
}

ReportLogWorker::~ReportLogWorker()
{
    qDebug() << "Destroying ReportLogWorker";

    qDeleteAll(logDataObj.begin(), logDataObj.end());
    logDataObj.clear();
    qDebug() << "Cleared all log data objects";

    if (logLibrary.isLoaded()) {
        logLibrary.unload();
        qDebug() << "Unloaded log library";
    }
}

bool ReportLogWorker::init()
{
    qDebug() << "Initializing ReportLogWorker";

    QList<ReportDataInterface *> datas {
        new StatusReportData,
        new FileDeliveryReportData,
        new ConnectionReportData
    };
    qDebug() << "Created default report data handlers";

    std::for_each(datas.cbegin(), datas.cend(), [this](ReportDataInterface *dat) { registerLogData(dat->type(), dat); });

    logLibrary.setFileName("deepin-event-log");
    if (!logLibrary.load()) {
        qWarning() << "Report log load log library failed!";
        return false;
    } else {
        qInfo() << "Report log load log library success.";
    }

    initEventLogFunc = reinterpret_cast<InitEventLog>(logLibrary.resolve("Initialize"));
    writeEventLogFunc = reinterpret_cast<WriteEventLog>(logLibrary.resolve("WriteEventLog"));
    qDebug() << "Resolved log library functions";

    if (!initEventLogFunc || !writeEventLogFunc) {
        qWarning() << "Log library init failed!";
        return false;
    }

    if (!initEventLogFunc(QApplication::applicationName().toStdString(), false)) {
        qWarning() << "Log library init function call failed!";
        return false;
    }

    qInfo() << "ReportLogWorker initialized successfully";
    return true;
}

void ReportLogWorker::commitLog(const QString &type, const QVariantMap &args)
{
    qDebug() << "Committing log for type:" << type << "with args:" << args;

    ReportDataInterface *interface = logDataObj.value(type, nullptr);
    if (!interface) {
        qInfo() << "Error: Log data object is not registed.";
        return;
    }

    QJsonObject jsonObject = interface->prepareData(args);
    qDebug() << "Prepared log data JSON object";

    const QStringList &keys = commonData.keys();
    foreach (const QString &key, keys) {
        jsonObject.insert(key, commonData.value(key));   //add common data for each log commit
    }
    qDebug() << "Added common data fields to log entry";

    commit(jsonObject.toVariantHash());
    qInfo() << "Successfully committed log for type:" << type;
}

bool ReportLogWorker::registerLogData(const QString &type, ReportDataInterface *dataObj)
{
    qDebug() << "Registering log data handler for type:" << type;

    if (logDataObj.contains(type)) {
        qWarning() << "Log data handler already registered for type:" << type;
        return false;
    }

    logDataObj.insert(type, dataObj);
    qInfo() << "Successfully registered log data handler for type:" << type;
    return true;
}

void ReportLogWorker::commit(const QVariant &args)
{
    qDebug() << "Finalizing log commit";

    if (args.isNull() || !args.isValid()) {
        qWarning() << "Invalid log data provided";
        return;
    }

    const QJsonObject &dataObj = QJsonObject::fromVariantHash(args.toHash());
    QJsonDocument doc(dataObj);
    const QByteArray &sendData = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Serialized log data to JSON";

    writeEventLogFunc(sendData.data());
    qInfo() << "Log data written to event log";
}
