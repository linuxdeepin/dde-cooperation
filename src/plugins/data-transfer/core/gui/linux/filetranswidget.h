#ifndef FILETRANSWIDGET_H
#define FILETRANSWIDGET_H

#include <QFrame>

class FileTransWidget : public QFrame
{
    Q_OBJECT

public:
    FileTransWidget(QWidget *parent = nullptr);
    ~FileTransWidget();

public slots:
    void nextPage();

private:
    void initUI();
    void initSelectFrame();
    QFrame *selectFrame = nullptr;
};

#endif
