#ifndef FILETRANSWIDGET_H
#define FILETRANSWIDGET_H

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMap>

class FileTransWidget : public QFrame
{
    Q_OBJECT

public:
    FileTransWidget(QWidget *parent = nullptr);
    ~FileTransWidget();

public slots:
    void nextPage();
    void update();

private:
    void initUI();
    void initSelectFrame();
    void sendOptions();

private:
    QFrame *selectFrame { nullptr };
    QGridLayout *selectLayout { nullptr };
    QLabel *storageInfoLabel { nullptr };

    QMap<QString, double> userData;
    qint64 remainStorage;
};

namespace Directory {
inline constexpr char kDocuments[] { "documents" };
inline constexpr char kMusic[] { "music" };
inline constexpr char kPicture[] { "picture" };
inline constexpr char kMovie[] { "movie" };
inline constexpr char kDownload[] { "download" };
}

#endif
