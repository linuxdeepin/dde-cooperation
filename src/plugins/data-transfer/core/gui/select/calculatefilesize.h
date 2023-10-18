#ifndef CALCULATEFILESIZE_H
#define CALCULATEFILESIZE_H

#include <QModelIndex>
#include <QObject>
#include <QRunnable>
#include <QThread>

class QListView;
class QThreadPool;
class QModelIndex;
class QTimer;

QString fromByteToQstring(quint64 bytes);
quint64 fromQstringToByte(QString sizeString);

class CalculateFileSizeTask : public QRunnable
{
public:
    CalculateFileSizeTask(QObject *pool, const QString &path, QListView *listview,
                          const QModelIndex &qindex);
    ~CalculateFileSizeTask() override;
    void run() override;
private:
    qlonglong calculate(const QString &path);
    QString filePath;
    qlonglong fileSize{ 0 };
    QListView *listView{ nullptr };
    QModelIndex index;
    QObject * calculatePool {nullptr};
};

class CalculateFileSizeThreadPool : public QObject
{
    Q_OBJECT
public:
    static CalculateFileSizeThreadPool *instance();
    ~CalculateFileSizeThreadPool();
    void init(const QList<QListView *> &list);
public slots:
    void sendFileSizeSlots(qlonglong fileSize, QListView *listview, QModelIndex index);
signals:
    void sendFileSizeSignal(qlonglong fileSize, QListView *listview, QModelIndex index);

private:
    CalculateFileSizeThreadPool();

    QThreadPool *threadPool;
    QList<QListView *> listView;
};


class CalculateFileSizeListen : public QObject
{
    Q_OBJECT
public:
    CalculateFileSizeListen(QObject *parent = nullptr);
    ~CalculateFileSizeListen();

    void addFileList(QListView *listView, QModelIndex index);
signals:
    void updateFileSize(QString size);
public slots:
    void doWork();

private:
    void calculate();
    bool done{ false };
    quint64 size{ 0 };
    QList<QPair<QListView *, QModelIndex>> fileLlist;

    QThread *thread{ nullptr };
};

#endif // CALCULATEFILESIZE_H
