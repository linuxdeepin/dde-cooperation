#include "userselectfilesize.h"
#include "calculatefilesize.h"
#include "common/log.h"

#include <QString>
#include <QDebug>
#include <QModelIndex>
#include <QListView>
#include <QStandardItemModel>

UserSelectFileSize::UserSelectFileSize()
{
    DLOG << "Initializing file size calculator";

    // Update the size of the selected files to be computed.
    QObject::connect(CalculateFileSizeThreadPool::instance(),
                     &CalculateFileSizeThreadPool::sendFileSizeSignal, this,
                     &UserSelectFileSize::updatependingFileSize);
}

void UserSelectFileSize::sendFileSize()
{
    if (pendingFiles.isEmpty()) {
        emit updateUserFileSelectSize(fromByteToQstring(userSelectFileSize));
    } else {
        emit updateUserFileSelectSize(QString(tr("Calculating")));
    }
}

UserSelectFileSize::~UserSelectFileSize()
{
    DLOG << "Destroying file size calculator";
}

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
    DLOG << "Adding pending file:" << path.toStdString();

    pendingFiles.push_back(path);
}

void UserSelectFileSize::delPendingFiles(const QString &path)
{
    DLOG << "Removing pending file:" << path.toStdString();

    if (pendingFiles.contains(path))
        pendingFiles.removeOne(path);
}

void UserSelectFileSize::addSelectFiles(const QString &path)
{
    DLOG << "Adding selected file:" << path.toStdString();

    selectFiles.push_back(path);
    emit updateUserFileSelectNum(path, true);
}

void UserSelectFileSize::delSelectFiles(const QString &path)
{
    DLOG << "Removing selected file:" << path.toStdString();

    if (selectFiles.contains(path))
        selectFiles.removeOne(path);
    emit updateUserFileSelectNum(path, false);
}

void UserSelectFileSize::addUserSelectFileSize(quint64 filesize)
{
    DLOG << "Adding file size:" << filesize << "bytes";

    userSelectFileSize += filesize;
    sendFileSize();
}

void UserSelectFileSize::delUserSelectFileSize(quint64 filesize)
{
    DLOG << "Removing file size:" << filesize << "bytes";

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
    DLOG << "Updating pending file size - Path:" << path.toStdString()
             << "Size:" << size << "bytes";

    if (pendingFiles.contains(path)) {
        userSelectFileSize += size;
        pendingFiles.removeOne(path);
        sendFileSize();
    }
}

void UserSelectFileSize::delDevice(QStandardItem *siderbarItem)
{
    DLOG << "Removing device from selection";

    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    QStringList::iterator it = selectFiles.begin();
    while (it != selectFiles.end()) {
        if (filemap->value(*it).siderbarItem == siderbarItem) {
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

void UserSelectFileSize::updateFileSelectList(QStandardItem *item)
{
    DLOG << "Updating file selection list for item";

    QString path = item->data(Qt::UserRole).toString();
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    if (item->data(Qt::CheckStateRole) == Qt::Unchecked) {
        if ((*filemap)[path].isSelect == false) {
            return;
        }
        // do not select the file
        (*filemap)[path].isSelect = false;
        delSelectFiles(path);
        if ((*filemap)[path].isCalculate) {
            quint64 size = (*filemap)[path].size;
            delUserSelectFileSize(size);
        } else {
            delPendingFiles(path);
        }
    } else if (item->data(Qt::CheckStateRole) == Qt::Checked) {
        if ((*filemap)[path].isSelect == true) {
            return;
        }
        (*filemap)[path].isSelect = true;
        addSelectFiles(path);
        if ((*filemap)[path].isCalculate) {
            quint64 size = (*filemap)[path].size;
            addUserSelectFileSize(size);
        } else {
            addPendingFiles(path);
        }
    }
}
