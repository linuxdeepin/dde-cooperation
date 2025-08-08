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
    qInfo() << "ReportLogWorker instance created";
}

ReportLogWorker::~ReportLogWorker()
{
    qInfo() << "Destroying ReportLogWorker, cleaning up resources";
    qDeleteAll(logDataObj.begin(), logDataObj.end());
    logDataObj.clear();

    if (logLibrary.isLoaded()) {
        qInfo() << "Unloading log library";
        logLibrary.unload();
    }
}

bool ReportLogWorker::init()
{
    qInfo() << "Initializing ReportLogWorker";
    QList<ReportDataInterface *> datas {
        new StatusReportData,
        new FileDeliveryReportData,
        new ConnectionReportData
    };

    qInfo() << "Registering log data types";
    std::for_each(datas.cbegin(), datas.cend(), [this](ReportDataInterface *dat) { registerLogData(dat->type(), dat); });

    logLibrary.setFileName("deepin-event-log");
    qInfo() << "Loading log library: " << logLibrary.fileName();
    if (!logLibrary.load()) {
        qWarning() << "Report log load log library failed!";
        return false;
    } else {
        qInfo() << "Log library loaded successfully";
    }

    qInfo() << "Resolving library functions";
    initEventLogFunc = reinterpret_cast<InitEventLog>(logLibrary.resolve("Initialize"));
    writeEventLogFunc = reinterpret_cast<WriteEventLog>(logLibrary.resolve("WriteEventLog"));

    if (!initEventLogFunc || !writeEventLogFunc) {
        qWarning() << "Log library init failed!";
        return false;
    }

    qInfo() << "Initializing log library with app name: " << QApplication::applicationName();
    if (!initEventLogFunc(QApplication::applicationName().toStdString(), false)) {
        qWarning() << "Log library init function call failed!";
        return false;
    }

    qInfo() << "Log worker initialized successfully";
    return true;
}

void ReportLogWorker::commitLog(const QString &type, const QVariantMap &args)
{
    qInfo() << "Committing log of type: " << type << " with args: " << args;
    ReportDataInterface *interface = logDataObj.value(type, nullptr);
    if (!interface) {
        qInfo() << "Error: Log data object is not registed.";
        return;
    }
    QJsonObject jsonObject = interface->prepareData(args);

    const QStringList &keys = commonData.keys();
    foreach (const QString &key, keys) {
        jsonObject.insert(key, commonData.value(key));   //add common data for each log commit
    }

    commit(jsonObject.toVariantHash());
}

bool ReportLogWorker::registerLogData(const QString &type, ReportDataInterface *dataObj)
{
    qInfo() << "Registering log data type: " << type;
    if (logDataObj.contains(type)) {
        qWarning() << "Error: Log data type is already registered.";
        return false;
    }

    logDataObj.insert(type, dataObj);
    return true;
}

void ReportLogWorker::commit(const QVariant &args)
{
    if (args.isNull() || !args.isValid()) {
        qInfo() << "Invalid log data, skipping commit";
        return;
    }
    const QJsonObject &dataObj = QJsonObject::fromVariantHash(args.toHash());
    QJsonDocument doc(dataObj);
    const QByteArray &sendData = doc.toJson(QJsonDocument::Compact);
    writeEventLogFunc(sendData.data());
    qInfo() << "Log data submitted successfully";
}
