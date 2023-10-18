#include "calculatefilesize.h"
#include <QThreadPool>
#include <QListView>
#include <QStandardItemModel>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QThread>

QString fromByteToQstring(quint64 bytes)
{
    float tempresult = static_cast<float>(bytes);
    float result = tempresult;
    if (tempresult < 1024) {
        return QString("%1B").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    if (result < 1024.0) {
        return QString("%1KB").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    if (result < 1024.0) {
        return QString("%1MB").arg(QString::number(result));
    }
    tempresult = tempresult / 1024;
    result = roundf(tempresult * 10) / 10;
    if (result < 1024.0) {
        return QString("%1GB").arg(QString::number(result));
    }
}
quint64 fromQstringToByte(QString sizeString)
{
    quint64 bytes;
    if (sizeString.endsWith("KB")) {
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024;
    } else if (sizeString.endsWith("MB")) {
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024 * 1024;
    } else if (sizeString.endsWith("GB")) {
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024 * 1024 * 1024;
    } else if (sizeString.endsWith("TB")) {
        sizeString.chop(2);
        bytes = sizeString.toDouble() * 1024 * 1024 * 1024 * 1024;
    } else if (sizeString.endsWith("B")) {
        sizeString.chop(1);
        bytes = sizeString.toDouble();
    }
    return bytes;
}

CalculateFileSizeTask::CalculateFileSizeTask(QObject *pool, const QString &path,
                                             QListView *listview, const QModelIndex &qindex)
    : filePath(path), listView(listview), index(qindex), calculatePool(pool)
{
}

CalculateFileSizeTask::~CalculateFileSizeTask() { }

void CalculateFileSizeTask::run()
{

    fileSize = calculate(filePath);
    QMetaObject::invokeMethod(calculatePool, "sendFileSizeSlots", Qt::QueuedConnection,
                              Q_ARG(qlonglong,fileSize), Q_ARG(QListView*, listView),
                              Q_ARG(QModelIndex, index));
}

qlonglong CalculateFileSizeTask::calculate(const QString &path)
{
    QDir directory(path);
    directory.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList fileList = directory.entryInfoList();
    qlonglong tempSize = 0;
    for (const QFileInfo &fileInfo : fileList) {
        if (fileInfo.isDir()) {
            tempSize += calculate(fileInfo.absoluteFilePath());
        } else {
            tempSize += fileInfo.size();
        }
    }
    return tempSize;
}

CalculateFileSizeThreadPool *CalculateFileSizeThreadPool::instance()
{
    static CalculateFileSizeThreadPool ins;
    return &ins;
}

CalculateFileSizeThreadPool::~CalculateFileSizeThreadPool()
{
    delete threadPool;
}

CalculateFileSizeThreadPool::CalculateFileSizeThreadPool()
{
    qRegisterMetaType<QListView*>("QListView*");

    threadPool = new QThreadPool();
}

void CalculateFileSizeThreadPool::init(const QList<QListView *> &list)
{
    listView = list;
    threadPool->setMaxThreadCount(4);
    for (QListView *view : listView) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(view->model());
        for (int row = 0; row < model->rowCount(); ++row) {
            QModelIndex index = model->index(row, 0);
            QString path = index.data(Qt::UserRole).toString();
            QFileInfo fileInfo(path);
            if (fileInfo.isFile()) {
                continue;
            } else if (fileInfo.isDir()) {
                CalculateFileSizeTask *task = new CalculateFileSizeTask(this,path, view, index);
                threadPool->start(task);
            } else {
                qWarning() << "Path is neither a file nor a directory:" << path;
            }
        }
    }
}

void CalculateFileSizeThreadPool::sendFileSizeSlots(qlonglong fileSize, QListView *listview,
                                                    QModelIndex index)
{
    emit sendFileSizeSignal(fileSize, listview, index);
}

CalculateFileSizeListen::CalculateFileSizeListen(QObject *parent) : QObject(parent) { }

CalculateFileSizeListen::~CalculateFileSizeListen() { }

void CalculateFileSizeListen::doWork()
{
    QTimer *timer = new QTimer(this);
    timer->moveToThread(thread);

    QObject::connect(timer, &QTimer::timeout, this, &CalculateFileSizeListen::calculate);
    QObject::connect(thread, &QThread::started, timer, [timer]() { timer->start(1000); });
    thread->start();
}

void CalculateFileSizeListen::calculate()
{
    for (QPair<QListView *, QModelIndex> value : fileLlist) {
        QListView *listview = value.first;
        QModelIndex index = value.second;
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listview->model());
        QString sizeStr = model->data(index, Qt::UserRole).toString();
        if (sizeStr != "") {
            fileLlist.removeOne(value);
            size += fromQstringToByte(sizeStr);
        }
    }
    if (fileLlist.size() == 0) {
        if (done == false) {
            done = true;
            emit updateFileSize(fromByteToQstring(size));
        }
    }
    emit updateFileSize(QString(""));
}

void CalculateFileSizeListen::addFileList(QListView *listView, QModelIndex index)
{
    fileLlist.push_back(qMakePair(listView, index));
    done = false;
}

