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
    initAppList();
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

    if (jsonObj.isEmpty())
        qWarning() << "this job none file";

    return jsonObj;
}

bool SettingHelper::handleDataConfiguration(const QString &filepath)
{
    addTaskcounter(1);
    QJsonObject jsonObj = ParseJson(filepath + "/" + "transfer.json");
    if (jsonObj.isEmpty()) {
        addTaskcounter(-1);
        isall = false;
        qWarning() << "transfer.json is invaild";
        emit TransferHelper::instance()->failure("配置文件", "文件", "配置文件错误或丢失");
        return false;
    }

    // Configure desktop wallpaper
    QString image = filepath + "/" + jsonObj["wallpapers"].toString();
    if (!jsonObj["wallpapers"].isNull())
        isall &= setWallpaper(image);

    //Configure file
    isall &= setFile(jsonObj, filepath);

    //setBrowserBookMark
    if (!jsonObj["browserbookmark"].toString().isEmpty())
        isall &= setBrowserBookMark(filepath + "/" + jsonObj["browserbookmark"].toString());

    //installApps
    QJsonValue userFileValue = jsonObj["app"];
    if (userFileValue.isArray()) {
        const QJsonArray &userFileArray = userFileValue.toArray();
        for (const auto &value : userFileArray) {
            isall &= installApps(value.toString());
        }
    }
    addTaskcounter(-1);
    QFile::remove(filepath + "/" + "transfer.json");
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
    QString targetDir = QDir::homePath() + "/.config/browser/Default/book/";
    QDir dir(targetDir);
    if (!dir.exists())
        dir.mkpath(".");

    QFileInfo info(filepath);
    if (info.suffix() != "json") {
        emit TransferHelper::instance()->failure("浏览器书签", "书签", "格式错误");
        return false;
    }

    QString targetfile = targetDir + info.fileName();
    qInfo() << "Set browser bookmarks" << filepath << targetfile;

    bool success = moveFile(filepath, targetfile);
    qInfo() << "Set browser bookmarks" << targetfile << success;
    if (!success) {
        emit TransferHelper::instance()->failure("浏览器书签", "书签", "设置失败，可手动导入配置");
        return false;
    }
    return true;
}

bool SettingHelper::installApps(const QString &app)
{
    if (app.isEmpty())
        return true;

    QString &package = applist[app];
    if (package.isEmpty()) {
        emit TransferHelper::instance()->failure(app, "应用", "安装失败，请进入应用商店安装");
        return false;
    }

    qInfo() << "Installing " << app << package;

    QString service = "com.deepin.lastore";
    QString path = "/com/deepin/lastore";
    QString interfaceName = "com.deepin.lastore.Manager";

    QDBusInterface interface(service, path, interfaceName, QDBusConnection::systemBus());

    //Check if installed
    QString existfunc = "PackageExists";
    QDBusMessage existReply = interface.call(existfunc, package);
    if (existReply.type() == QDBusMessage::ReplyMessage) {
        bool isExist = existReply.arguments().at(0).toBool();
        if (isExist) {
            qWarning() << app << "is installed";
            return true;
        }
    }

    //installed
    QString func = "InstallPackage";

    QDBusMessage reply = interface.call(func, QString(), package);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qWarning() << "Installing " << app << "false" << reply.errorMessage();
        emit TransferHelper::instance()->failure(app, "应用", app + "安装失败，请进入应用商店安装");
        return false;
    }

    QString jobPath = reply.arguments().at(0).value<QDBusObjectPath>().path();
    qInfo() << "Installing " << app << "true" << jobPath;

    bool success = QDBusConnection::systemBus().connect(service, jobPath, "org.freedesktop.DBus.Properties",
                                                        "PropertiesChanged", this, SLOT(onPropertiesChanged(QDBusMessage)));
    if (!success)
        qWarning() << "Failed to connect to signal";

    emit TransferHelper::instance()->transferContent("正在安装", app, 100, -2);

    addTaskcounter(1);
    return true;
}

void SettingHelper::onPropertiesChanged(const QDBusMessage &message)
{
    if (message.arguments().count() != 3)
        return;
    QVariantMap changedProps = qdbus_cast<QVariantMap>(message.arguments().at(1).value<QDBusArgument>());
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
        QString app = applist.key(package);
        QString content = applist.key(package) + "  Key:" + key + "   Value:" + value.toString();
        qInfo() << content;
        emit TransferHelper::instance()->transferContent("正在安装", content, 100, -2);
        if (key == "Status" && value == "succeed")
            addTaskcounter(-1);
        if (key == "Status" && value == "failed") {
            addTaskcounter(-1);
            isall = false;
            emit TransferHelper::instance()->failure(package, "应用", "安装失败，请进入应用商店安装");
        }
    }
}

void SettingHelper::addTaskcounter(int value)
{
    if(taskcounter == 0)
        init();

    taskcounter += value;

    if (taskcounter == 0) {
        emit TransferHelper::instance()->transferContent("", "迁移完成", 100, -1);
        emit TransferHelper::instance()->transferSucceed(isall);
    }
}

void SettingHelper::init()
{
    isall = true;

    //clear
    emit TransferHelper::instance()->failure("", "clear", "");
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
            auto dir = QFileInfo(targetFile).dir();
            if(!dir.exists())
                dir.mkpath(".");
            moveFile(file, targetFile);
        }
    }
    qInfo() << jsonObj["user_file"].toString();
    return true;
}

bool SettingHelper::moveFile(const QString &src, QString &dst)
{
    QFileInfo srcFileInfo(src);
    QString dstDir = QFileInfo(dst).path();
    if (QFile::exists(dst)) {
        int i = 1;
        QString baseName = srcFileInfo.baseName();
        QString suffix = srcFileInfo.completeSuffix();
        if (!suffix.isEmpty())
            suffix = "." + suffix;
        while (QFile::exists(dst)) {
            dst = dstDir + "/" + baseName + "(" + QString::number(i) + ")" + suffix;
            i++;
        }
    }
    QFile f(src);
    qInfo() << dst;
    if (f.rename(dst))
        return true;

    qWarning() << f.errorString();
    return false;
}

void SettingHelper::initAppList()
{
    QJsonObject jsonObj = ParseJson(":/fileResource/apps.json");
    if (jsonObj.isEmpty())
        return;
    for (const QString &app : jsonObj.keys()) {
        applist[app] = jsonObj.value(app).toObject().value("packageName").toString();
    }
    qInfo() << "SettingHelper::initAppList() finished";
}
