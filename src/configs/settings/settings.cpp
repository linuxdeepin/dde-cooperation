// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settings.h"

#include <QFileSystemWatcher>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QThread>
#include <QUrl>

class SettingsPrivate
{
public:
    explicit SettingsPrivate(Settings *qq);

    bool autoSync = false;
    bool watchChanges = false;
    bool settingFileIsDirty = false;

    QTimer *syncTimer = nullptr;

    QString fallbackFile;
    QString settingFile;
    QFileSystemWatcher *settingFileWatcher = nullptr;

    Settings *q_ptr;

    struct Data
    {
        QHash<QString, QVariantHash> values;
        QHash<QString, QVariantHash> privateValues;

        QVariant value(const QString &group, const QString &key, const QVariant &dv = QVariant()) const
        {
            return values.value(group).value(key, dv);
        }

        void setValue(const QString group, const QString &key, const QVariant &value)
        {
            if (!values.contains(group)) {
                values.insert(group, { { key, value } });

                return;
            }

            values[group][key] = value;
        }

        QVariantMap groupMetaData(const QString &group) const
        {
            return privateValues.value("__metadata__").value(group).toMap();
        }

        QStringList groupKeyOrderedList(const QString &group) const
        {
            return groupMetaData(group).value("keyOrdered").toStringList();
        }
    };

    Data defaultData;
    Data fallbackData;
    Data writableData;

    void fromJsonFile(const QString &fileName, Data *data);
    void fromJson(const QByteArray &json, Data *data);
    QByteArray toJson(const Data &data);

    void makeSettingFileToDirty(bool dirty)
    {
        qInfo() << "Setting dirty flag to:" << dirty;
        if (settingFileIsDirty == dirty) {
            return;
        }

        settingFileIsDirty = dirty;

        if (!autoSync) {
            return;
        }

        Q_ASSERT(syncTimer);

        if (QThread::currentThread() == syncTimer->thread()) {
            if (dirty) {
                qInfo() << "makeSettingFileToDirty start";
                syncTimer->start();
            } else {
                qInfo() << "makeSettingFileToDirty stop";
                syncTimer->stop();
            }
        } else {
            qInfo() << "makeSettingFileToDirty invokeMethod";
            syncTimer->metaObject()->invokeMethod(syncTimer, dirty ? "start" : "stop", Qt::QueuedConnection);
        }
    }

    void _q_onFileChanged(const QString &filePath);
};

SettingsPrivate::SettingsPrivate(Settings *qq)
    : q_ptr(qq)
{
    qInfo() << "Constructing SettingsPrivate";
}

void SettingsPrivate::fromJsonFile(const QString &fileName, Data *data)
{
    qInfo() << "Loading settings from JSON file:" << fileName;
    QFile file(fileName);

    if (!file.exists()) {
        qInfo() << "File not exists:" << fileName;
        return;
    }

    if (!file.open(QFile::ReadOnly)) {
        qInfo() << "Failed to open file:" << fileName << file.errorString();
        qWarning() << file.errorString();

        return;
    }

    const QByteArray &json = file.readAll();

    if (json.isEmpty()) {
        qInfo() << "File is empty:" << fileName;
        return;
    }

    fromJson(json, data);
}

void SettingsPrivate::fromJson(const QByteArray &json, Data *data)
{
    qInfo() << "Parsing JSON data";
    QJsonParseError error;
    const QJsonDocument &doc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError) {
        qInfo() << "Json parse error:" << error.errorString();
        qWarning() << error.errorString();
        return;
    }

    if (!doc.isObject()) {
        qInfo() << "Json is not an object";
        qWarning() << QString();
        return;
    }

    const QJsonObject &groups_object = doc.object();

    for (auto begin = groups_object.constBegin(); begin != groups_object.constEnd(); ++begin) {
        const QJsonValue &value = begin.value();

        if (!value.isObject()) {
            qInfo() << "Value is not an object";
            qWarning() << QString();
            continue;
        }

        const QJsonObject &value_object = value.toObject();
        QVariantHash hash;

        for (auto iter = value_object.constBegin(); iter != value_object.constEnd(); ++iter) {
            hash[iter.key()] = iter.value().toVariant();
        }

        // private groups
        if (begin.key().startsWith("__") && begin.key().endsWith("__"))
            data->privateValues[begin.key()] = hash;
        else
            data->values[begin.key()] = hash;
    }
}

QByteArray SettingsPrivate::toJson(const Data &data)
{
    qInfo() << "Converting settings data to JSON";
    QJsonObject root_object;

    for (auto begin = data.values.constBegin(); begin != data.values.constEnd(); ++begin) {
        root_object.insert(begin.key(), QJsonValue(QJsonObject::fromVariantHash(begin.value())));
    }

    return QJsonDocument(root_object).toJson();
}

void SettingsPrivate::_q_onFileChanged(const QString &filePath)
{
    qInfo() << "Handling file change notification for:" << filePath;
    if (filePath != settingFile)
        return;

    const auto old_values = writableData.values;

    writableData.values.clear();
    fromJsonFile(settingFile, &writableData);
    makeSettingFileToDirty(false);

    for (auto begin = writableData.values.constBegin(); begin != writableData.values.constEnd(); ++begin) {
        for (auto i = begin.value().constBegin(); i != begin.value().constEnd(); ++i) {
            if (old_values.value(begin.key()).contains(i.key())) {
                if (old_values.value(begin.key()).value(i.key()) == i.value()) {
                    continue;
                }
            } else {
                if (fallbackData.values.value(begin.key()).contains(i.key())) {
                    if (fallbackData.values.value(begin.key()).value(i.key()) == i.value()) {
                        continue;
                    }
                }

                if (defaultData.values.value(begin.key()).value(i.key()) == i.value()) {
                    continue;
                }
            }

            Q_EMIT q_ptr->valueEdited(begin.key(), i.key(), i.value());
            Q_EMIT q_ptr->valueChanged(begin.key(), i.key(), i.value());
        }
    }

    for (auto begin = old_values.constBegin(); begin != old_values.constEnd(); ++begin) {
        for (auto i = begin.value().constBegin(); i != begin.value().constEnd(); ++i) {
            if (writableData.values.value(begin.key()).contains(i.key())) {
                continue;
            }

            const QVariant &new_value = q_ptr->value(begin.key(), i.key());

            if (new_value != old_values.value(begin.key()).value(i.key())) {
                Q_EMIT q_ptr->valueEdited(begin.key(), i.key(), new_value);
                Q_EMIT q_ptr->valueChanged(begin.key(), i.key(), new_value);
            }
        }
    }
}

/*!
 * \class DFMSettings
 * \inmodule dde-file-manager-lib
 *
 * \brief DFMSettings provide interfaces to access and modify the file manager setting options.
 */
Settings::Settings(const QString &defaultFile, const QString &fallbackFile, const QString &settingFile, QObject *parent)
    : QObject(parent), d_ptr(new SettingsPrivate(this))
{
    qInfo() << "Constructing Settings with file paths:" << defaultFile << fallbackFile << settingFile;
    d_ptr->fallbackFile = fallbackFile;
    d_ptr->settingFile = settingFile;

    d_ptr->fromJsonFile(defaultFile, &d_ptr->defaultData);
    d_ptr->fromJsonFile(fallbackFile, &d_ptr->fallbackData);
    d_ptr->fromJsonFile(settingFile, &d_ptr->writableData);
}

static QString getConfigFilePath(QStandardPaths::StandardLocation type, const QString &fileName, bool writable)
{
    qInfo() << "Getting config file path";
    if (writable) {
        QString path = QStandardPaths::writableLocation(type);

        if (path.isEmpty()) {
            path = QDir::home().absoluteFilePath(QString(".config/%1/%2").arg(qApp->organizationName()).arg(qApp->applicationName()));
        }
        qInfo() << "Get writable config file path:" << path;
        return path.append(QString("/%1.json").arg(fileName));
    }

    const QStringList &list = QStandardPaths::standardLocations(type);

    QString path = list.isEmpty() ? QString("/etc/xdg/%1/%2").arg(qApp->organizationName()).arg(qApp->applicationName()) : list.last();
    qInfo() << "Get read-only config file path:" << path;
    return path.append(QString("/%1.json").arg(fileName));
}

Settings::Settings(const QString &name, ConfigType type, QObject *parent)
    : Settings(QString(":/config/%1.json").arg(name),
                  getConfigFilePath(type == AppConfig
                                            ? QStandardPaths::AppConfigLocation
                                            : QStandardPaths::GenericConfigLocation,
                                    name, false),
                  getConfigFilePath(type == AppConfig
                                            ? QStandardPaths::AppConfigLocation
                                            : QStandardPaths::GenericConfigLocation,
                                    name, true),
                  parent)
{
    qInfo() << "Constructing Settings with name";
}

Settings::~Settings()
{
    qInfo() << "Destructing Settings";
    Q_D(Settings);

    if (d->syncTimer) {
        qInfo() << "Stop sync timer";
        d->syncTimer->stop();
    }

    if (d->settingFileIsDirty) {
        qInfo() << "Sync settings on exit";
        sync();
    }
}

bool Settings::contains(const QString &group, const QString &key) const
{
    qInfo() << "Checking if settings contain group:" << group << "key:" << key;
    Q_D(const Settings);

    if (key.isEmpty()) {
        if (d->writableData.values.contains(group)) {
            qInfo() << "Group exists in writable data:" << group;
            return true;
        }

        if (d->fallbackData.values.contains(group)) {
            qInfo() << "Group exists in fallback data:" << group;
            return true;
        }
        qInfo() << "Check group in default data:" << group;
        return d->defaultData.values.contains(group);
    }

    if (d->writableData.values.value(group).contains(key)) {
        qInfo() << "Key exists in writable data:" << group << key;
        return true;
    }

    if (d->fallbackData.values.value(group).contains(key)) {
        qInfo() << "Key exists in fallback data:" << group << key;
        return true;
    }
    qInfo() << "Check key in default data:" << group << key;
    return d->defaultData.values.value(group).contains(key);
}

QSet<QString> Settings::groups() const
{
    qInfo() << "Getting all group names";
    Q_D(const Settings);

    QSet<QString> groups;

    groups.reserve(d->writableData.values.size() + d->fallbackData.values.size() + d->defaultData.values.size());

    for (auto begin = d->writableData.values.constBegin(); begin != d->writableData.values.constEnd(); ++begin) {
        groups << begin.key();
    }

    for (auto begin = d->fallbackData.values.constBegin(); begin != d->fallbackData.values.constEnd(); ++begin) {
        groups << begin.key();
    }

    for (auto begin = d->defaultData.values.constBegin(); begin != d->defaultData.values.constEnd(); ++begin) {
        groups << begin.key();
    }
    qInfo() << "Get all groups, count:" << groups.size();
    return groups;
}

QSet<QString> Settings::keys(const QString &group) const
{
    qInfo() << "Getting all keys for group:" << group;
    Q_D(const Settings);

    QSet<QString> keys;

    const auto &&wg = d->writableData.values.value(group);
    const auto &&fg = d->fallbackData.values.value(group);
    const auto &&dg = d->defaultData.values.value(group);

    keys.reserve(wg.size() + fg.size() + dg.size());

    for (auto begin = wg.constBegin(); begin != wg.constEnd(); ++begin) {
        keys << begin.key();
    }

    for (auto begin = fg.constBegin(); begin != fg.constEnd(); ++begin) {
        keys << begin.key();
    }

    for (auto begin = dg.constBegin(); begin != dg.constEnd(); ++begin) {
        keys << begin.key();
    }
    qInfo() << "Get all keys for group:" << group << "count:" << keys.size();
    return keys;
}

/*!
 * \brief DFMSettings::keysList
 * \param group name
 * \return An ordered key list of the group
 */
QStringList Settings::keyList(const QString &group) const
{
    qInfo() << "Getting ordered key list for group:" << group;
    Q_D(const Settings);

    QStringList keyList;
    QSet<QString> keys = this->keys(group);

    for (const QString &ordered_key : d->defaultData.groupKeyOrderedList(group)) {
        if (keys.contains(ordered_key)) {
            keyList << ordered_key;
            keys.remove(ordered_key);
        }
    }

    for (const QString &ordered_key : d->fallbackData.groupKeyOrderedList(group)) {
        if (keys.contains(ordered_key)) {
            keyList << ordered_key;
            keys.remove(ordered_key);
        }
    }

    for (const QString &ordered_key : d->writableData.groupKeyOrderedList(group)) {
        if (keys.contains(ordered_key)) {
            keyList << ordered_key;
            keys.remove(ordered_key);
        }
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    keyList << keys.toList();
#else
    keyList << QStringList(keys.begin(), keys.end());
#endif
    qInfo() << "Get ordered key list for group:" << group << "count:" << keyList.size();
    return keyList;
}

QVariant Settings::value(const QString &group, const QString &key, const QVariant &defaultValue) const
{
    qInfo() << "Getting value for group:" << group << "key:" << key;
    Q_D(const Settings);

    QVariant value = d->writableData.values.value(group).value(key, QVariant());

    if (value.isValid()) {
        qInfo() << "Get value from writable data:" << group << key;
        return value;
    }

    value = d->fallbackData.values.value(group).value(key, QVariant());

    if (value.isValid()) {
        qInfo() << "Get value from fallback data:" << group << key;
        return value;
    }
    qInfo() << "Get value from default data:" << group << key;
    return d->defaultData.values.value(group).value(key, defaultValue);
}

void Settings::setValue(const QString &group, const QString &key, const QVariant &value)
{
    qInfo() << "Setting value for group:" << group << "key:" << key;
    if (setValueNoNotify(group, key, value)) {
        qInfo() << "Value changed, emit signal";
        Q_EMIT valueChanged(group, key, value);
    }
}

bool Settings::setValueNoNotify(const QString &group, const QString &key, const QVariant &value)
{
    qInfo() << "Setting value without notification for group:" << group << "key:" << key;
    Q_D(Settings);

    bool changed = false;

    if (isRemovable(group, key)) {
        if (d->writableData.value(group, key) == value) {
            qInfo() << "Value not changed";
            return false;
        }

        changed = true;
    } else {
        changed = this->value(group, key) != value;
    }

    d->writableData.setValue(group, key, value);
    d->makeSettingFileToDirty(true);

    return changed;
}

void Settings::removeGroup(const QString &group)
{
    qInfo() << "Removing group:" << group;
    Q_D(Settings);

    if (!d->writableData.values.contains(group)) {
        qInfo() << "Group not exists in writable data:" << group;
        return;
    }

    const QVariantHash &group_values = d->writableData.values.take(group);

    d->makeSettingFileToDirty(true);
    qInfo() << "Remove group:" << group;

    for (auto begin = group_values.constBegin(); begin != group_values.constEnd(); ++begin) {
        const QVariant &new_value = value(group, begin.key());

        if (new_value != begin.value()) {
            qInfo() << "Value changed, emit signal";
            Q_EMIT valueChanged(group, begin.key(), new_value);
        }
    }
}

bool Settings::isRemovable(const QString &group, const QString &key) const
{
    qInfo() << "Checking if key is removable for group:" << group << "key:" << key;
    Q_D(const Settings);
    bool result = d->writableData.values.value(group).contains(key);
    qInfo() << "Check if key is removable:" << group << key << result;
    return result;
}

void Settings::remove(const QString &group, const QString &key)
{
    qInfo() << "Removing key for group:" << group << "key:" << key;
    Q_D(Settings);

    if (!d->writableData.values.value(group).contains(key)) {
        qInfo() << "Key not exists in writable data:" << group << key;
        return;
    }

    const QVariant &old_value = d->writableData.values[group].take(key);
    d->makeSettingFileToDirty(true);
    qInfo() << "Remove key:" << group << key;

    const QVariant &new_value = value(group, key);

    if (old_value == new_value) {
        qInfo() << "Value not changed:" << group << key;
        return;
    }

    qInfo() << "Value changed, emit signal";
    Q_EMIT valueChanged(group, key, new_value);
}

void Settings::clear()
{
    qInfo() << "Clearing all writable settings";
    Q_D(Settings);

    if (d->writableData.values.isEmpty()) {
        qInfo() << "Writable data is empty, no need to clear";
        return;
    }

    const QHash<QString, QVariantHash> old_values = d->writableData.values;

    d->writableData.values.clear();
    d->makeSettingFileToDirty(true);
    qInfo() << "Clear all writable data";

    for (auto begin = old_values.constBegin(); begin != old_values.constEnd(); ++begin) {
        const QVariantHash &values = begin.value();

        for (auto i = values.constBegin(); i != values.constEnd(); ++i) {
            const QVariant &new_value = value(begin.key(), i.key());

            if (new_value != i.value()) {
                qInfo() << "Value changed, emit signal";
                Q_EMIT valueChanged(begin.key(), i.key(), new_value);
            }
        }
    }
}

/*!
 * \brief Reload config file.
 *
 * This will be needed if file watcher is disabled, or say, you defined the
 * DFM_NO_FILE_WATCHER marco.
 */
void Settings::reload()
{
    qInfo() << "Reloading all settings from files";
    Q_D(Settings);

    d->fallbackData.privateValues.clear();
    d->fallbackData.values.clear();
    d->fromJsonFile(d->fallbackFile, &d_ptr->fallbackData);

    d->writableData.privateValues.clear();
    d->writableData.values.clear();
    d->fromJsonFile(d->settingFile, &d_ptr->writableData);
}

bool Settings::sync()
{
    qInfo() << "Syncing settings to file";
    Q_D(Settings);

    if (!d->settingFileIsDirty) {
        qInfo() << "Settings not dirty, no need to sync";
        return true;
    }

    const QByteArray &json = d->toJson(d->writableData);

    QFile file(d->settingFile);

    if (!file.open(QFile::WriteOnly)) {
        qInfo() << "Failed to open file for writing:" << d->settingFile << file.errorString();
        return false;
    }

    bool ok = file.write(json) == json.size();

    if (ok) {
        qInfo() << "Sync settings to file successfully:" << d->settingFile;
        d->makeSettingFileToDirty(false);
    } else {
        qInfo() << "Failed to write to file:" << d->settingFile;
    }
    file.close();

    return ok;
}

bool Settings::autoSync() const
{
    qInfo() << "Checking if auto sync is enabled";
    Q_D(const Settings);

    return d->autoSync;
}

bool Settings::watchChanges() const
{
    qInfo() << "Checking if watch changes is enabled";
    Q_D(const Settings);

    return d->watchChanges;
}

void Settings::setAutoSync(bool autoSync)
{
    qInfo() << "Setting auto sync to:" << autoSync;
    Q_D(Settings);

    if (d->autoSync == autoSync) {
        return;
    }

    d->autoSync = autoSync;
    qInfo() << "Set auto sync to:" << autoSync;

    if (autoSync) {
        if (d->settingFileIsDirty) {
            qInfo() << "Sync dirty settings immediately";
            sync();
        }

        if (!d->syncTimer) {
            qInfo() << "Create sync timer";
            d->syncTimer = new QTimer(this);
            d->syncTimer->moveToThread(thread());
            d->syncTimer->setSingleShot(true);
            d->syncTimer->setInterval(1000);

            connect(d->syncTimer, &QTimer::timeout, this, &Settings::sync);
        }
    } else {
        if (d->syncTimer) {
            qInfo() << "Destroy sync timer";
            d->syncTimer->stop();
            d->syncTimer->deleteLater();
            d->syncTimer = nullptr;
        }
    }
}

void Settings::onFileChanged(const QString &filePath)
{
    qInfo() << "File changed handler, path:" << filePath;
    Q_D(Settings);

    d->_q_onFileChanged(filePath);
}

void Settings::setWatchChanges(bool watchChanges)
{
    qInfo() << "Setting watch changes to:" << watchChanges;
    Q_D(Settings);

    if (d->watchChanges == watchChanges)
        return;

    d->watchChanges = watchChanges;
    qInfo() << "Set watch changes to:" << watchChanges;
    if (watchChanges) {
        {
            QFileInfo info(d->settingFile);

            if (!info.exists()) {
                qInfo() << "Setting file not exists, create it:" << d->settingFile;
                if (info.absoluteDir().mkpath(info.absolutePath())) {
                    QFile file(d->settingFile);
                    file.open(QFile::WriteOnly);
                }
            }
        }

        qInfo() << "Create file watcher for:" << d->settingFile;
        d->settingFileWatcher = new QFileSystemWatcher({ d->settingFile }, this);
        d->settingFileWatcher->moveToThread(thread());

        connect(d->settingFileWatcher, &QFileSystemWatcher::fileChanged, this, &Settings::onFileChanged);
    } else {
        if (d->settingFileWatcher) {
            qInfo() << "Destroy file watcher";
            d->settingFileWatcher->deleteLater();
            d->settingFileWatcher = nullptr;
        }
    }
}
