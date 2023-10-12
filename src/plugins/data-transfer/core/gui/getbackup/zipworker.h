#ifndef ZIPWORKER_H
#define ZIPWORKER_H

#include <QThread>
class ZipWork : public QThread
{
    Q_OBJECT
public:
    ZipWork();
    ~ZipWork() override;

    void run() override;

private:
    void zipFile(const QStringList &sourceFilePath,const QString &zipFileSave = QString());
    void unZipFile(const QString &zipFilePath,const QString &unZipFile = QString());
    void getJsonfile(const QJsonObject &jsonData, const QString &save);

    void getUserDataPackagingFile();

};
#endif
