#ifndef UNZIPWORKER_H
#define UNZIPWORKER_H

#include <QJsonObject>
#include <QString>
#include <QThread>

class UnzipWorker : public QThread
{
public:
    UnzipWorker(QString filepath);
    ~UnzipWorker() override;

    void run() override;

    bool extract();
    bool set();

private:
    bool setUesrFile(QJsonObject jsonObj);
    QString filepath;
    QString targetDir;
    QThread workerThread;
};

#endif
