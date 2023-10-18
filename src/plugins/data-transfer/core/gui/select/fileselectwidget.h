#ifndef FILESELECTWIDGET_H
#define FILESELECTWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QMap>

#include "../select/selectmainwidget.h"

class QHBoxLayout;
class QStackedWidget;
class SelectListView;
class SidebarWidget : public QListView
{
    Q_OBJECT
public:
    SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initData();
};

class FileSelectWidget : public QFrame
{
    Q_OBJECT

public:
    FileSelectWidget(QListView *siderbarWidget, QWidget *parent = nullptr);
    ~FileSelectWidget();

    void initFileView();
    void changeText();

public slots:
    void nextPage();
    void backPage();
    void updateFileView(const QModelIndex &index);
    void selectOrDelAllItem();
    void updateFilesize(qlonglong fileSize, QListView *listview, QModelIndex index);

   // void updateSideFilesize(qlonglong fileSize, QModelIndex index);

signals:
    void isOk(const SelectItemName &name, const bool &ok);

private:
    void initUI();
    void sendOptions();
    SelectListView *getFileView(const QString &path);

    void createFilesizeListen(QListView* listView);

private:
    QListView *sidebar{ nullptr };

    QMap<QString, quint8> fileViewList;
    QStackedWidget *stackedWidget{ nullptr };
    QLabel *titileLabel{ nullptr };

    bool aync = true;
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
