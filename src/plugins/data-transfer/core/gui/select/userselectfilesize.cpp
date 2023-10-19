#include "userselectfilesize.h"
#include "calculatefilesize.h"
#include <QString>
#include <QDebug>
#include <QModelIndex>
#include <QListView>

#pragma execution_character_set("utf-8")
UserSelectFileSize::UserSelectFileSize() { }

void UserSelectFileSize::sendFileSize()
{
    if (pendingFiles.isEmpty()) {
        emit updateUserFileSelectSize(fromByteToQstring(userSelectFileSize));
    } else {
        emit updateUserFileSelectSize(QString("计算中"));
    }
}

UserSelectFileSize::~UserSelectFileSize() { }

UserSelectFileSize *UserSelectFileSize::instance()
{
    static UserSelectFileSize ins;
    return &ins;
}

bool UserSelectFileSize::done()
{
    return pendingFiles.empty();
}

bool UserSelectFileSize::isPendingFile(const QString &path)
{
    return pendingFiles.contains(path);
}

void UserSelectFileSize::addPendingFiles(const QString &path)
{
    pendingFiles.push_back(path);
}

void UserSelectFileSize::delPendingFiles(const QString &path)
{
    if (pendingFiles.contains(path))
        pendingFiles.removeAll(path);
}

void UserSelectFileSize::addSelectFiles(const QString &path)
{
    selectFiles.push_back(path);
    sendFileSize();
}

void UserSelectFileSize::delSelectFiles(const QString &path)
{
    if (selectFiles.contains(path))
        selectFiles.removeAll(path);
}

void UserSelectFileSize::addUserSelectFileSize(quint64 filesize)
{
    userSelectFileSize += filesize;
    sendFileSize();
}

void UserSelectFileSize::delUserSelectFileSize(quint64 filesize)
{
    userSelectFileSize -= filesize;
    sendFileSize();
}


quint64 UserSelectFileSize::getAllSelectSize()
{
    return userSelectFileSize;
}

QStringList UserSelectFileSize::getSelectFilesList()
{
    return selectFiles;
}

void UserSelectFileSize::updatependingFileSize(const quint64 &size, const QString &path)
{
    if (pendingFiles.contains(path)) {
        userSelectFileSize += size;
        pendingFiles.removeAll(path);
        sendFileSize();
    }
}
