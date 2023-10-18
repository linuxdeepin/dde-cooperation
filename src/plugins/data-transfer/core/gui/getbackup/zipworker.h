#ifndef ZIPWORKER_H
#define ZIPWORKER_H

#include <QThread>

class QElapsedTimer;
class ZipWork : public QThread
{
    Q_OBJECT
public:
    ZipWork();
    ~ZipWork() override;

    void run() override;

private:
    void zipFile(const QStringList &sourceFilePath, const QString &zipFileSave = QString());
    void unZipFile(const QString &zipFilePath, const QString &unZipFile = QString());

    void getUserDataPackagingFile();

    int getPathFileNum(const QString &filePath);
    int getAllFileNum(const QStringList &fileList);

private:
    QElapsedTimer timer;
    int allFileNum{ 0 };
    int zipFileNum{ 0 };
    int lastZipFileNum{ 0 };
};
#endif
