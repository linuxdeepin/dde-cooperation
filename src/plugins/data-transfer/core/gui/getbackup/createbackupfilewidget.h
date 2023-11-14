#ifndef CREATEBACKUPFILEWIDGET_H
#define CREATEBACKUPFILEWIDGET_H

#include <QFrame>
#include <QMap>

class QListView;
class QLineEdit;
class QLabel;
class QStorageInfo;
class QStandardItem;
class QToolButton;
class CreateBackupFileWidget : public QFrame
{
    Q_OBJECT
public:
    CreateBackupFileWidget(QWidget *parent = nullptr);
    ~CreateBackupFileWidget();

    void sendOptions();

private:
    void initUI();
    void initDiskListView();
    void checkDisk();
    void setDetermineButtonEnable(bool enable);

public slots:
    void nextPage();
    void backPage();
    void updateuserSelectFileSize(const QString &sizeStr);
    void updaeBackupFileSize();
    void getUpdateDeviceSingla();
    void updateDevice(const QStorageInfo &device, const bool &isAdd);

private:
    quint64 userSelectFileSize{ 0 };
    quint64 allSize{ 0 };
    QString backupFileName{ "" };
    QLabel *promptLabel{ nullptr };
    QLabel *backupFileSizeLabel{ nullptr };
    QListView *diskListView{ nullptr };
    QLineEdit *fileNameInput{ nullptr };
    QToolButton *determineButton{ nullptr };
    QList<QStorageInfo> deviceList;

    QMap<QStandardItem *, quint64> diskCapacity;
};

#endif // CREATEBACKUPFILEWIDGET_H
