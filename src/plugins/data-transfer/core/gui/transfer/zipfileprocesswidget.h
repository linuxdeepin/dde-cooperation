#ifndef ZIPFILEPROCESSWIDGET_H
#define ZIPFILEPROCESSWIDGET_H

#include <QFrame>

class QLabel;
class ProgressBarLabel;
class zipFileProcessWidget : public QFrame
{
public:
    zipFileProcessWidget(QWidget *parent = nullptr);
    ~zipFileProcessWidget();
public slots:
    void updateProcess(const QString &content, int processbar, int estimatedtime);

private:
    void changeFileLabel(const QString &path);
    void changeTimeLabel(const int &time);
    void changeProgressBarLabel(const int &processbar);
    void initUI();
    void nextPage();
private:
    QLabel *fileLabel{ nullptr };
    QLabel *timeLabel{ nullptr };
    ProgressBarLabel *progressLabel{ nullptr };
};

#endif // ZIPFILEPROCESSWIDGET_H
