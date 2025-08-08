// SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationreportdata.h"

#include <DSysInfo>

#include <QDateTime>
#include <QDebug>

using namespace deepin_cross;
DCORE_USE_NAMESPACE

static QVariantMap mergeCommonAttributes(const QVariantMap &args)
{
    qDebug() << "Merging common attributes for report data";
    QVariantMap map = args;

    if (DSysInfo::isDeepin()) {
        qDebug() << "Running on Deepin system, adding version info";
        map.insert("systemVersion", DSysInfo::uosEditionName());
        map.insert("versionNumber", DSysInfo::minorVersion());
    }
    map.insert("sysTime", QDateTime::currentDateTime().toString("yyyy/MM/dd"));
    map.insert("machineID", QSysInfo::machineUniqueId());

    return map;
}

QString StatusReportData::type() const
{
    qDebug() << "Getting report data type: CooperationStatus";
    return "CooperationStatus";
}

QJsonObject StatusReportData::prepareData(const QVariantMap &args) const
{
    qDebug() << "Preparing cooperation status report data";
    QVariantMap data = mergeCommonAttributes(args);
    data.insert("tid", 1000800000);
    qDebug() << "Added transaction ID for status report";

    QJsonObject result = QJsonObject::fromVariantMap(data);
    qDebug() << "Status report data prepared successfully";
    return result;
}

QString FileDeliveryReportData::type() const
{
    qDebug() << "Getting report data type: FileDelivery";
    return "FileDelivery";
}

QJsonObject FileDeliveryReportData::prepareData(const QVariantMap &args) const
{
    qDebug() << "Preparing file delivery report data";
    QVariantMap data = mergeCommonAttributes(args);
    data.insert("tid", 1000800001);
    qDebug() << "Added transaction ID for file delivery report";

    QJsonObject result = QJsonObject::fromVariantMap(data);
    qDebug() << "File delivery report data prepared successfully";
    return result;
}

QString ConnectionReportData::type() const
{
    qDebug() << "Getting report data type: ConnectionInfo";
    return "ConnectionInfo";
}

QJsonObject ConnectionReportData::prepareData(const QVariantMap &args) const
{
    qDebug() << "Preparing connection info report data";
    QVariantMap data = mergeCommonAttributes(args);
    data.insert("tid", 1000800002);
    qDebug() << "Added transaction ID for connection info report";

    QJsonObject result = QJsonObject::fromVariantMap(data);
    qDebug() << "Connection info report data prepared successfully";
    return result;
}
