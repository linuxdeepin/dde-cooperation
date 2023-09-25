#ifndef CREATEBACKUPFILEWIDGET_H
#define CREATEBACKUPFILEWIDGET_H

#include <QFrame>

class CreateBackupFileWidget : public QFrame
{
    Q_OBJECT
public:
    CreateBackupFileWidget(QWidget *parent = nullptr);
    ~CreateBackupFileWidget();

public slots:
    void nextPage();
    void backPage();

private:
    QString backupFileName{ "" };
    QString backupFileSize{ "0GB" };
};

#endif // CREATEBACKUPFILEWIDGET_H
