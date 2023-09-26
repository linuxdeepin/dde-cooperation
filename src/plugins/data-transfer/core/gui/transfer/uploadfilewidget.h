#ifndef UPLOADFILEWIDGET_H
#define UPLOADFILEWIDGET_H

#include <QFrame>

class UploadFileWidget : public QFrame
{
    Q_OBJECT

public:
    UploadFileWidget(QWidget *parent = nullptr);
    ~UploadFileWidget();

public slots:
    void nextPage();
    void backPage();

private:
    void initUI();
};

class UploadFileFrame : public QFrame
{
    Q_OBJECT

public:
    UploadFileFrame(QWidget *parent = nullptr);
    ~UploadFileFrame();

    void uploadFile();
    void update();

private:
    void initUI();

private:
    QString selectedFilePath;
};

#endif
