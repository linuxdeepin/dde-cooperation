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

#include "../select/selectmainwidget.h"

class FileSelectWidget : public QFrame
{
    Q_OBJECT

public:
    FileSelectWidget(QListView *siderbarWidget, QWidget *parent = nullptr);
    ~FileSelectWidget();

    void initFileView();
    void initSiderBar(QListView *siderbarWidget);
    void initConnect(QAbstractItemView *view);
    void changeText();
    void updateFileView();
public slots:
    void nextPage();
    void backPage();

    void update();
signals:
    void isOk(const SelectItemName &name, const bool &ok);

private:
    void initUI();
    void sendOptions();

private:
    QListView *sidebar{ nullptr };
    QListView *fileview{ nullptr };
    QLabel *titileLabel{ nullptr };
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

class SidebarItemDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    SidebarItemDelegate() { }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return QSize(170, 36);
    }
};

namespace Directory {
inline constexpr char kMovie[]{ "视频" };
inline constexpr char kPicture[]{ "图片" };
inline constexpr char kMusic[]{ "音乐" };
inline constexpr char kDocuments[]{ "文档" };
inline constexpr char kDownload[]{ "下载" };
inline constexpr char kDesktop[]{ "桌面" };
} // namespace Directory

#endif
