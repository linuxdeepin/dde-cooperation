#ifndef CREATEBACKUPFILEWIDGET_H
#define CREATEBACKUPFILEWIDGET_H

#include <QFrame>

class QListView;
class QLineEdit;
class QLabel;
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
public slots:
    void nextPage();
    void backPage();
    void updateuserSelectFileSize(const QString &sizeStr);
    void updaeBackupFileSize();
private:
    quint64 userSelectFileSize{0};

    QString backupFileName{ "" };

    QLabel *backupFileSizeLabel{nullptr};
    QListView *diskListView{ nullptr };
    QLineEdit *fileNameInput{ nullptr };
};

#endif // CREATEBACKUPFILEWIDGET_H
