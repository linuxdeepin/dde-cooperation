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
    void updateBackupFileSize(const QString &sizeStr);
private:
    QString backupFileName{ "" };

    QLabel *backupFileSizeLabel{nullptr};
    QListView *diskListView{ nullptr };
    QLineEdit *fileNameInput{ nullptr };
};

#endif // CREATEBACKUPFILEWIDGET_H
