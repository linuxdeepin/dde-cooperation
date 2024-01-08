﻿#ifndef SETTINGHELPER_H
#define SETTINGHELPER_H

#include <QMap>
#include <QObject>
#include <QDBusMessage>
#include <QUrl>

class SettingHelper : public QObject
{
    Q_OBJECT

public:
    SettingHelper();
    ~SettingHelper();

    static SettingHelper *instance();

    static QJsonObject ParseJson(const QString &filepath);
    static bool moveFile(const QString &src, QString &dst);

public:
    bool handleDataConfiguration(const QString &filepath);
    bool setWallpaper(const QString &filepath);
    bool setBrowserBookMark(const QString &filepath);
    bool installApps(const QString &app);
    bool setFile(QJsonObject jsonObj, QString filepath);

    void addTaskcounter(int value);

    void init();
public Q_SLOTS:
    void onPropertiesChanged(const QDBusMessage &message);

private:
    void initAppList();

private:
    //用于统计开启了多少个配置任务。
    int taskcounter = 0;

    //用于记录是否有任务失败。
    bool isall = true;

    //App and package list
    QMap<QString, QString> applist;
};

#endif
