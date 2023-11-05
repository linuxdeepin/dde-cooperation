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
class QStorageInfo;
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

   void updateAllSizeUi(const quint64 &size, const bool &isAdd);
public slots:
    void updateSelectSizeUi(const QString &sizeStr);
    void updateSiderbarFileSize(quint64 fileSize, const QString &path);
    void getUpdateDeviceSingla();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initData();
    void initUi();
    void updateDevice(const QStorageInfo &device, const bool &isAdd);

    void initSiderbarSize();
    void initSiderbarUi();

    void updateSiderbarSize(QModelIndex index, quint64 size);
    void updateSiderbarUi(QModelIndex index);

    void updatePorcessLabel();

    void updateAllSize();
signals:
    void updateFileview(const QModelIndex &siderIndex, const bool &isAdd);

private:
    QMap<QModelIndex, quint64> sidebarSizeList;
    QLabel *userSelectFileSize{ nullptr };
    ProgressBarLabel *processLabel{ nullptr };

    QList<QStorageInfo> deviceList;

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

    void changeText();

public slots:
    void nextPage();
    void backPage();

    void changeFileView(const QModelIndex &index);
    void selectOrDelAllItem();
    void updateFileSelectList(QStandardItem *item);
    void updateFileViewSize(quint64 fileSize, const QString &path);

    void updateFileViewData(const QModelIndex &siderIndex, const bool &isAdd);
signals:
    void isOk(const SelectItemName &name);

private:
    void initUI();
    void initFileView();
    void sendOptions();
    void delOptions();
    SelectListView *addFileViewData(const QString &path, const QModelIndex &siderbarIndex);
    void createFilesizeListen(QListView *listView);

    void startCalcluateFileSize(QList<QString> fileList);

private:
    SidebarWidget *sidebar{ nullptr };
    QMap<QModelIndex, QListView *> sidebarFileViewList;
    QStackedWidget *stackedWidget{ nullptr };
    QLabel *titileLabel{ nullptr };
};

#endif
