#include "unzipwoker.h"

#include <QProcess>
#include <QDebug>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <utils/transferhepler.h>

#include <QTimer>
#include <zip.h>

inline constexpr char datajson[] { "transfer.json" };

UnzipWorker::UnzipWorker(QString filepath)
    : filepath(filepath)
{
    QFileInfo fileInfo(filepath);
    targetDir = fileInfo.path() + "/" + fileInfo.baseName();
    count = getNumFiles(filepath);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        speed = currentTotal - previousTotal + 1;
        previousTotal = currentTotal;
    });

    //Calculate transmission speed
    timer->start(500);
}

UnzipWorker::~UnzipWorker()
{
}

void UnzipWorker::run()
{
    //decompression
    extract();

    //configuration
    TransferHelper::instance()->setting(targetDir);
}

int UnzipWorker::getNumFiles(QString filepath)
{
    const char *zipFilePath = filepath.toLocal8Bit().constData();
    struct zip *archive = zip_open(zipFilePath, 0, NULL);

    if (archive) {
        int fileCount = zip_get_num_files(archive);
        qInfo() << "Number of files in ZIP file:" << fileCount;

        zip_close(archive);
        return fileCount;
    } else {
        qInfo() << "Unable to open ZIP file";
        return 0;
    }
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
            currentTotal++;
            double value = static_cast<double>(currentTotal) / count;
            int progressbar = static_cast<int>(value * 100);
            int estimatedtime = (count - currentTotal) / speed / 2 + 1;
            emit TransferHelper::instance()->transferContent("正在解压" + outputText, progressbar, estimatedtime);
            qInfo() << value << outputText;
        }
    }
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
