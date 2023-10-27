#ifndef FILESELECTWIDGET_H
#define FILESELECTWIDGET_H

#include <QFrame>
#include <QListView>
#include <QMap>
#include <QStandardItemModel>
#include "../select/selectmainwidget.h"

class QLabel;
class QHBoxLayout;
class QStackedWidget;
class SelectListView;
class ProgressBarLabel;
class SidebarWidget : public QListView
{
    Q_OBJECT
public:
    SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

    void updateUserSelectFileSizeUi();
    QMap<QModelIndex, quint64> *getSidebarSizeList();

    void initSiderDataAndUi();
    void updateSiderDataAndUi(QModelIndex index, quint64 size);
public slots:
    void updateSelectSizeUi(const QString &sizeStr);
    void updateAllSizeUi();

    void updateSiderbarFileSize(quint64 fileSize, const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initData();
    void initUi();

    void initSiderbarSize();
    void initSiderbarUi();
    void updateSiderbarSize(QModelIndex index, quint64 size);
    void updateSiderbarUi(QModelIndex index);

    void updatePorcessLabel();

private:
    QMap<QModelIndex, quint64> sidebarSizeList;
    QLabel *userSelectFileSize{ nullptr };
    ProgressBarLabel *processLabel{ nullptr };

    QString selectSizeStr{ "0B" };
    QString allSizeStr{ "0B" };
    quint64 allSize{ 0 };
};

class FileSelectWidget : public QFrame
{
    Q_OBJECT

public:
    FileSelectWidget(SidebarWidget *siderbarWidget, QWidget *parent = nullptr);
    ~FileSelectWidget();
    void initFileView();
    void changeText();

public slots:
    void nextPage();
    void backPage();

    void changeFileView(const QModelIndex &index);
    void selectOrDelAllItem();
    void updateFileSelectList(QStandardItem *item);
    void updateFileViewSize(quint64 fileSize, const QString &path);

signals:
    void isOk(const SelectItemName &name);

private:
    void initUI();
    void sendOptions();
    void delOptions();
    SelectListView *initFileView(const QString &path, const QModelIndex &siderbarIndex);
    void createFilesizeListen(QListView *listView);

    void startCalcluateFileSize();

private:
    SidebarWidget *sidebar{ nullptr };
    QMap<QModelIndex, QListView *> sidebarFileViewList;
    QMap<QModelIndex, quint64> *sidebarSizeList;
    QStackedWidget *stackedWidget{ nullptr };
    QLabel *titileLabel{ nullptr };
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
