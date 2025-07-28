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
    QVariantMap map = args;
    qInfo() << "Merging common report attributes";

    if (DSysInfo::isDeepin()) {
        qInfo() << "Deepin system detected, adding version info";
        map.insert("systemVersion", DSysInfo::uosEditionName());
        map.insert("versionNumber", DSysInfo::minorVersion());
    }
    qInfo() << "Adding system time and machine ID";
    map.insert("sysTime", QDateTime::currentDateTime().toString("yyyy/MM/dd"));
    map.insert("machineID", QSysInfo::machineUniqueId());

    return map;
}

QString StatusReportData::type() const
{
    qInfo() << "Getting report type: CooperationStatus";
    return "CooperationStatus";
}

QJsonObject StatusReportData::prepareData(const QVariantMap &args) const
{
    qInfo() << "Preparing status report data";
    QVariantMap data = mergeCommonAttributes(args);
    data.insert("tid", 1000800000);
    qInfo() << "Status report data prepared with tid: 1000800000";
    return QJsonObject::fromVariantMap(data);
}

QString FileDeliveryReportData::type() const
{
    qInfo() << "Getting report type: FileDelivery";
    return "FileDelivery";
}

QJsonObject FileDeliveryReportData::prepareData(const QVariantMap &args) const
{
    qInfo() << "Preparing file delivery report data";
    QVariantMap data = mergeCommonAttributes(args);
    data.insert("tid", 1000800001);
    qInfo() << "File delivery report data prepared with tid: 1000800001";
    return QJsonObject::fromVariantMap(data);
}

QString ConnectionReportData::type() const
{
    qInfo() << "Getting report type: ConnectionInfo";
    return "ConnectionInfo";
}

QJsonObject ConnectionReportData::prepareData(const QVariantMap &args) const
{
    qInfo() << "Preparing connection report data";
    QVariantMap data = mergeCommonAttributes(args);
    data.insert("tid", 1000800002);
    qInfo() << "Connection report data prepared with tid: 1000800002";
    return QJsonObject::fromVariantMap(data);
}
