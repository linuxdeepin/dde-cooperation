#include "settinghepler.h"
#include "transferhepler.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QJsonDocument>

SettingHelper::SettingHelper()
    : QObject()
{
}

SettingHelper::~SettingHelper() {}

SettingHelper *SettingHelper::instance()
{
    static SettingHelper ins;
    return &ins;
}

QJsonObject SettingHelper::ParseJson(const QString &filepath)
{
    QJsonObject jsonObj;
    QFile file(filepath);
    qInfo() << "Parsing the configuration file for transmission" << file.fileName();
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "could not open datajson file";
        return jsonObj;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        qWarning() << "Parsing JSON data failed";
        return jsonObj;
    }
    jsonObj = jsonDoc.object();

    return jsonObj;
}

bool SettingHelper::handleDataConfiguration(const QString &filepath)
{
    addTaskcounter(1);
    QJsonObject jsonObj = ParseJson(filepath + "/" + "transfer.json");
    if (jsonObj.isEmpty())
        return false;

    // Configure desktop wallpaper
    QString image = filepath + "/" + jsonObj["wallpapers"].toString();
    if (!jsonObj["wallpapers"].isNull())
        isall &= setWallpaper(image);

    //Configure file
    isall &= setFile(jsonObj, filepath);

    //setBrowserBookMark
    isall &= setBrowserBookMark(filepath + "/" + jsonObj["browerbookmark"].toString());

    //installApps
    QJsonValue userFileValue = jsonObj["app"];
    if (userFileValue.isArray()) {
        const QJsonArray &userFileArray = userFileValue.toArray();
        for (const auto &value : userFileArray) {
            isall &= installApps(value.toString());
        }
    }
    addTaskcounter(-1);
    return isall;
}

bool SettingHelper::setWallpaper(const QString &filepath)
{
    qInfo() << "Setting picture as wallpaper" << filepath;

    QString service = "com.deepin.daemon.Appearance";
    QString path = "/com/deepin/daemon/Appearance";
    QString interfaceName = "com.deepin.daemon.Appearance";

    QDBusInterface interface(service, path, interfaceName);

    QString func = "SetMonitorBackground";
    QString screenName = QGuiApplication::screens().first()->name();
    QVariant monitorName = QVariant::fromValue(screenName);
    QVariant imageFile = QVariant::fromValue(filepath);

    QDBusMessage reply = interface.call(func, monitorName, imageFile);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        qDebug() << "SetMonitorBackground method called successfully";
        return true;
    } else {
        qDebug() << "Failed to call SetMonitorBackground method";
        return false;
    }
}

bool SettingHelper::setBrowserBookMark(const QString &filepath)
{
    if (filepath.isEmpty())
        return true;
    qInfo() << "Set browser bookmarks" << filepath;
    QString targetDir = QDir::homePath() + "/.config/browser/Default/book/";
    QString targetFile = targetDir + filepath.section('/', -1);
    QDir dir(targetDir);
    if (!dir.exists())
        dir.mkpath(".");
    bool success = QFile::rename(filepath, targetFile);
    qInfo() << "Set browser bookmarks" << targetFile << success;
    if (!success) {
        emit TransferHelper::instance()->failure("浏览器书签", "书签", "设置失败");
        return false;
    }
    return true;
}

bool SettingHelper::installApps(const QString &app)
{
    if (app.isEmpty())
        return true;
    qInfo() << "Installing " << app;

    QString service = "com.deepin.lastore";
    QString path = "/com/deepin/lastore";
    QString interfaceName = "com.deepin.lastore.Manager";

    QDBusInterface interface(service, path, interfaceName, QDBusConnection::systemBus());

    QString func = "InstallPackage";

    QDBusMessage reply = interface.call(func, QString(), app);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qWarning() << "Installing " << app << "false" << reply.errorMessage();
        emit TransferHelper::instance()->failure("应用安装", "应用", "暂不支持");
        return false;
    }

    QString jobPath = reply.arguments().at(0).value<QDBusObjectPath>().path();
    qInfo() << "Installing " << app << "true" << jobPath;

    bool success = QDBusConnection::systemBus().connect(service, jobPath, "org.freedesktop.DBus.Properties",
                                                        "PropertiesChanged", this, SLOT(onPropertiesChanged(QDBusMessage)));
    if (!success)
        qWarning() << "Failed to connect to signal";

    addTaskcounter(1);
    return true;
}

void SettingHelper::onPropertiesChanged(const QDBusMessage &message)
{
    if (message.arguments().count() != 3)
        return;
    QVariantMap changedProps = qdbus_cast<QVariantMap>(message.arguments().at(1).value<QDBusArgument>());
    qInfo() << changedProps;
    foreach (const QString &key, changedProps.keys()) {
        QVariant value = changedProps.value(key);
        QDBusInterface interface("com.deepin.lastore",
                                 message.path(),
                                 "com.deepin.lastore.Job",
                                 QDBusConnection::systemBus());
        auto packages = interface.property("Packages").toStringList();
        QString package;
        if (!packages.isEmpty())
            package = packages.first();
        QString content = package + "  Key:" + key + "   Value:" + value.toString();
        qInfo() << content;
        emit TransferHelper::instance()->transferContent("正在安装" + content, 99, 1);
        if (key == "Status" && value == "succeed")
            addTaskcounter(-1);
    }
}

void SettingHelper::addTaskcounter(int value)
{
    taskcounter += value;
    if (taskcounter == 0) {
        emit TransferHelper::instance()->transferSucceed(isall);
    }
}

bool SettingHelper::setFile(QJsonObject jsonObj, QString filepath)
{
    QJsonValue userFileValue = jsonObj["user_file"];
    if (userFileValue.isArray()) {
        const QJsonArray &userFileArray = userFileValue.toArray();
        for (const auto &value : userFileArray) {
            QString filename = value.toString();
            QString targetFile = QDir::homePath() + "/" + filename;
            QString file = filepath + filename.mid(filename.indexOf('/'));
            bool success = QFile::rename(file, targetFile);
            qInfo() << file << success;
        }
    }
    qInfo() << jsonObj["user_file"].toString();
    return true;
}
