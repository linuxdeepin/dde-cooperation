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
    ~UploadFileFrame() override;

    void uploadFile();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void updateUI(int status);

private:
    void initUI();
    void initStyleSheet();
    void initFileFrame();

private:
    QString selectedFilePath;
    QFrame *fileFrame { nullptr };
};

enum uploadStatus {
    invalid = 0,
    valid,
    formaterror
};

#endif
