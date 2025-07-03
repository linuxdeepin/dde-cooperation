#include "calculatefilesize.h"

#include "common/log.h"

#include <QThreadPool>
#include <QListView>
#include <QStandardItemModel>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <math.h>
#include <QMutex>

QString fromByteToQstring(quint64 bytes)
{
    DLOG << "Converting bytes to string";
    float tempresult = static_cast<float>(bytes);
    float result = tempresult;
    if (tempresult < 100.0) {
        DLOG << "Result is less than 100, returning in B";
        return QString("%1B").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    if (result < 100.0) {
        DLOG << "Result is less than 100KB, returning in KB";
        return QString("%1KB").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    if (result < 100.0) {
        DLOG << "Result is less than 100MB, returning in MB";
        return QString("%1MB").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    if (result < 100.0) {
        DLOG << "Result is less than 100GB, returning in GB";
        return QString("%1GB").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    return QString("%1TB").arg(QString::number(result));
}
quint64 fromQstringToByte(QString sizeString)
{
    DLOG << "Converting string to bytes";
    quint64 bytes = 0;
    if (sizeString.endsWith("KB")) {
        DLOG << "Converting KB to bytes";
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024;
    } else if (sizeString.endsWith("MB")) {
        DLOG << "Converting MB to bytes";
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024 * 1024;
    } else if (sizeString.endsWith("GB")) {
        DLOG << "Converting GB to bytes";
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024 * 1024 * 1024;
    } else if (sizeString.endsWith("TB")) {
        DLOG << "Converting TB to bytes";
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024 * 1024 * 1024 * 1024;
    } else if (sizeString.endsWith("B")) {
        DLOG << "Converting B to bytes";
        sizeString.chop(1);
        bytes = sizeString.toDouble();
    }
    return bytes;
}

CalculateFileSizeTask::CalculateFileSizeTask(QObject *pool, const QString &path)
    : filePath(path), calculatePool(pool)
{
    DLOG << "Creating calculation task for:" << filePath.toStdString();
}

CalculateFileSizeTask::~CalculateFileSizeTask() { }

void CalculateFileSizeTask::run()
{
    DLOG<< "Starting calculation for:" << filePath.toStdString();

    fileSize = calculate(filePath);
    QMetaObject::invokeMethod(calculatePool, "sendFileSizeSlots", Qt::QueuedConnection,
                              Q_ARG(quint64, fileSize), Q_ARG(QString, filePath));
}

void CalculateFileSizeTask::abortTask()
{
    DLOG<< "Aborting calculation for:" << filePath.toStdString();

    abort = true;
}

qlonglong CalculateFileSizeTask::calculate(const QString &path)
{
    DLOG << "Starting calculation for path:" << path.toStdString();
    if (abort) {
        DLOG << "Calculation aborted for path:" << path.toStdString();
        return 0;
    }

    QDir directory(path);
    directory.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList fileList = directory.entryInfoList();
    qlonglong tempSize = 0;
    for (const QFileInfo &fileInfo : fileList) {
        if (fileInfo.isDir()) {
            DLOG << "Calculating size for directory:" << fileInfo.absoluteFilePath().toStdString();
            tempSize += calculate(fileInfo.absoluteFilePath());
        } else {
            DLOG << "Calculating size for file:" << fileInfo.absoluteFilePath().toStdString();
            tempSize += fileInfo.size();
        }
    }

    DLOG << "Calculation completed for path:" << path.toStdString()
             << "Size:" << tempSize;
    return tempSize;
}

CalculateFileSizeThreadPool *CalculateFileSizeThreadPool::instance()
{
    static CalculateFileSizeThreadPool ins;
    return &ins;
}

CalculateFileSizeThreadPool::~CalculateFileSizeThreadPool()
{
    DLOG<< "Destroying thread pool";
}

CalculateFileSizeThreadPool::CalculateFileSizeThreadPool()
{
    DLOG<< "Initializing thread pool with max 4 threads";

    threadPool = new QThreadPool();
    fileMap = new QMap<QString, FileInfo>();
    threadPool->setMaxThreadCount(4);
    // connect main thread exit signal
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, this,
                     &CalculateFileSizeThreadPool::exitPool, Qt::DirectConnection);
}

void CalculateFileSizeThreadPool::work(const QList<QString> &list)
{
    DLOG<< "Starting work on" << list.size();

    for (const QString &path : list) {
        QFileInfo fileInfo(path);
        if (fileInfo.isFile()) {
            DLOG << "Path is a file, skipping:" << path.toStdString();
            continue;
        } else if (fileInfo.isDir()) {
            DLOG << "Path is a directory, starting new task:" << path.toStdString();
            CalculateFileSizeTask *task = new CalculateFileSizeTask(this, path);
            workList.push_back(task);
            threadPool->start(task);
        } else {
            WLOG << "Path is neither a file nor a directory:" << path.toStdString();
            DLOG << "Path is neither a file nor a directory:" << path.toStdString();
        }
    }
}

void CalculateFileSizeThreadPool::addFileMap(const QString &path, const FileInfo &fileinfo)
{
    DLOG<< "Adding file to map:" << path.toStdString();

    (*fileMap)[path] = fileinfo;
}

void CalculateFileSizeThreadPool::delFileMap(const QString &path)
{
    DLOG<< "Removing file from map:" << path.toStdString();

    if (fileMap->contains(path)) {
        DLOG << "File found in map, removing:" << path.toStdString();
        fileMap->remove(path);
    } else {
        DLOG << "File not found in map:" << path.toStdString();
    }
}

QMap<QString, FileInfo> *CalculateFileSizeThreadPool::getFileMap()
{
    return fileMap;
}

void CalculateFileSizeThreadPool::sendFileSizeSlots(quint64 fileSize, const QString &path)
{
    DLOG<< "Sending file size for" << path.toStdString()
             << "Size:" << fileSize;

    if (!fileMap->contains(path)) {
        DLOG << "File not found in map:" << path.toStdString();
        return;
    }

    (*fileMap)[path].size = fileSize;
    (*fileMap)[path].isCalculate = true;
    emit sendFileSizeSignal(fileSize, path);
}

void CalculateFileSizeThreadPool::addFileSlots(const QList<QString> &list)
{
    DLOG << "Adding files to thread pool:" << list.size();
    work(list);
}

void CalculateFileSizeThreadPool::exitPool()
{
    DLOG<< "Exiting thread pool, aborting"
             << workList.size() << "tasks";

    threadPool->clear();
    for (CalculateFileSizeTask *task : workList) {
        task->abortTask();
    }
    threadPool->waitForDone();
    LOG << "calculate file size exit.";
    delete threadPool;
    delete fileMap;
}

void CalculateFileSizeThreadPool::delDevice(const QStandardItem *siderbarItem)
{
    DLOG<< "Removing device entries for sidebar item";

    QMap<QString, FileInfo>::iterator it = fileMap->begin();
    while (it != fileMap->end()) {
        if (it.value().siderbarItem == siderbarItem) {
            // DLOG << "Removing entry for sidebar item:" << it.key().toStdString();
            it = fileMap->erase(it);
        } else {
            ++it;
        }
    }
}
