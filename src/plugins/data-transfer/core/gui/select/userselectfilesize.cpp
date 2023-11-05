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
        pendingFiles.removeOne(path);
}

void UserSelectFileSize::addSelectFiles(const QString &path)
{
    selectFiles.push_back(path);
    sendFileSize();
}

void UserSelectFileSize::delSelectFiles(const QString &path)
{
    if (selectFiles.contains(path))
        selectFiles.removeOne(path);
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
        pendingFiles.removeOne(path);
        sendFileSize();
    }
}

void UserSelectFileSize::delDevice(const QModelIndex &index)
{
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    QStringList::iterator it = selectFiles.begin();
    while (it != selectFiles.end()) {
        if (filemap->value(*it).siderIndex == index) {
            if (filemap->value(*it).isCalculate) {
                userSelectFileSize -= filemap->value(*it).size;
            }
            it = selectFiles.erase(it);
        } else {
            ++it;
        }
    }
    sendFileSize();
}
