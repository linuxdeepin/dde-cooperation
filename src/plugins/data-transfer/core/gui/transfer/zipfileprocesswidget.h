#ifndef ZIPFILEPROCESSWIDGET_H
#define ZIPFILEPROCESSWIDGET_H

#include <QFrame>

class QLabel;
class zipFileProcessWidget : public QFrame
{
public:
    zipFileProcessWidget(QWidget *parent = nullptr);
    ~zipFileProcessWidget();
public slots:
    void changeFileLabel(const QString &path);
    void changeTimeLabel(const QString &time);

private:
    void initUI();

private:
    QLabel *fileLabel{ nullptr };
    QLabel *timeLabel{ nullptr };
};

#endif // ZIPFILEPROCESSWIDGET_H
