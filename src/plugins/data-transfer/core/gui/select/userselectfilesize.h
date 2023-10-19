#ifndef USERSELECTFILESIZE_H
#define USERSELECTFILESIZE_H

#include <QObject>
#include <QMultiMap>
class QString;
class QListView;
class QModelIndex;
class UserSelectFileSize : public QObject
{
    Q_OBJECT
public:
    ~UserSelectFileSize();
    static UserSelectFileSize *instance();
    bool done();
    bool isPendingFile(const QString &path);
    void addPendingFiles(const QString &path);
    void delPendingFiles(const QString &path);
    void addSelectFiles(const QString &path);
    void delSelectFiles(const QString &path);

    void addUserSelectFileSize(quint64 filesize);
    void delUserSelectFileSize(quint64 filesize);

    void clearAllFileSelect();
    quint64 getAllSelectSize();
    QStringList getSelectFilesList();
signals:
    void updateUserFileSelectSize(const QString &size);
public slots:
    void updatependingFileSize(const quint64 &size,const QString &path);
private:
    UserSelectFileSize();
    void sendFileSize();
private:
    QStringList pendingFiles;

    QStringList selectFiles;

    quint64 userSelectFileSize{ 0 };
};

#endif // USERSELECTFILESIZE_H
