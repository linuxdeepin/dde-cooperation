#ifndef FILESELECTWIDGET_H
#define FILESELECTWIDGET_H

#include <QFrame>
#include <QGridLayout>
#include <QItemDelegate>
#include <QLabel>
#include <QListView>
#include <QMap>
#include <QPainter>
#include <QTreeView>

class FileSelectWidget : public QFrame
{
    Q_OBJECT

public:
    FileSelectWidget(QListView *siderbarWidget, QWidget *parent = nullptr);
    ~FileSelectWidget();

    void initFileView();
    void initSiderBar(QListView *siderbarWidget);
    void initConnect(QAbstractItemView *view);

    void updateFileView();
public slots:
    void nextPage();
    void backPage();

    void update();

private:
    void initUI();
    void sendOptions();

private:
    QListView *sidebar { nullptr };
    QListView *fileview { nullptr };
    QStringList seletFileList;
    bool aync = true;
};

class SidebarWidget : public QListView
{
    Q_OBJECT
public:
    SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

private:
    void initData();
};

namespace Directory {
inline constexpr char kMovie[] { "视频" };
inline constexpr char kPicture[] { "图片" };
inline constexpr char kMusic[] { "音乐" };
inline constexpr char kDocuments[] { "文档" };
inline constexpr char kDownload[] { "下载" };
inline constexpr char kDesktop[] { "桌面" };
}

#endif
