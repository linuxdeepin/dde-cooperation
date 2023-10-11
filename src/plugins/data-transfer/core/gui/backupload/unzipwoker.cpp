#include "unzipwoker.h"

#include <QProcess>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <utils/transferhepler.h>

inline constexpr char datajson[] { "transfer.json" };

UnzipWorker::UnzipWorker(QString filepath)
    : filepath(filepath)
{
    QFileInfo fileInfo(filepath);
    targetDir = fileInfo.path() + "/" + fileInfo.baseName();
}

UnzipWorker::~UnzipWorker()
{
}

void UnzipWorker::run()
{
    //decompression
    extract();
    //configuration
    //set();
}

bool UnzipWorker::extract()
{
    QStringList arguments;
    arguments << "-O"
              << "utf-8"
              << filepath
              << "-d"
              << targetDir;

    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setReadChannelMode(QProcess::SeparateChannels);
    process.start("unzip", arguments);

    qInfo() << process.arguments();
    while (process.waitForReadyRead()) {
        QByteArray output = process.readAllStandardOutput();
        QString outputText = QString::fromLocal8Bit(output);
        if (outputText.startsWith("  inflating: ")) {
            outputText = outputText.mid(outputText.indexOf("inflating:") + QString("inflating:").length() + 1);
            emit TransferHelper::instance()->transferContent(outputText, 0, 500);
        }
        qInfo() << outputText;
    }
    emit TransferHelper::instance()->transferContent("迁移完成！！！", 100, 0);
    if (process.exitCode() != 0)
        qInfo() << "Error message:" << process.errorString();
    process.waitForFinished();
    return true;
}

bool UnzipWorker::set()
{
    QFile file(targetDir + "/" + datajson);
    qInfo() << targetDir + datajson;
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "could not open datajson file";
        return false;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull()) {
        qWarning() << "Parsing JSON data failed";
        return false;
    }
    QJsonObject jsonObj = jsonDoc.object();

    //Place the file in the corresponding user directory
    setUesrFile(jsonObj);

    return true;
}

bool UnzipWorker::setUesrFile(QJsonObject jsonObj)
{
    QJsonValue userFileValue = jsonObj["user_file"];
    if (userFileValue.isArray()) {
        const QJsonArray &userFileArray = userFileValue.toArray();
        for (const auto &value : userFileArray) {
            QString file = value.toString();
            QString targetFile = QDir::homePath() + "/" + file;
            QString filepath = targetDir + file.mid(file.indexOf('/'));
            bool success = QFile::rename(filepath, targetFile);
            qInfo() << filepath << success;
        }
    }
    qInfo() << jsonObj["user_file"].toString();
    return true;
}
