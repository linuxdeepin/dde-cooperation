#ifndef CREATEBACKUPFILEWIDGET_H
#define CREATEBACKUPFILEWIDGET_H

#include <QFrame>

class QListView;
class QLineEdit;
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
    QString fromByteToGBorMB(quint64 bytes);
public slots:
    void nextPage();
    void backPage();

private:
    QString backupFileName{ "" };
    QString backupFileSize{ "0GB" };

    QListView *diskListView{ nullptr };
    QLineEdit *fileNameInput{ nullptr };
};

#endif // CREATEBACKUPFILEWIDGET_H
