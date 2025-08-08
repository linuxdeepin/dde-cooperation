#include "calculatefilesize.h"

#include "siderbarwidget.h"
#include "userselectfilesize.h"
#include <gui/win/devicelistener.h>
#include "common/log.h"

#include <QPainter>
#include <QDir>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QDebug>
#include <QLabel>
#include <QPainterPath>

#include <gui/transfer/transferringwidget.h>
#include <utils/optionsmanager.h>
#include <net/helper/transferhepler.h>
SidebarWidget::SidebarWidget(QWidget *parent) : QListView(parent)
{
    DLOG << "Initializing sidebar widget";

    UserPath[tr("Videos")] = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    UserPath[tr("Pictures")] = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    UserPath[tr("Documents")] = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    UserPath[tr("Downloads")] = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    UserPath[tr("Music")] = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    UserPath[tr("Desktop")] = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    initData();
    initUi();

    // update siderbar file size
    QObject::connect(CalculateFileSizeThreadPool::instance(),
                     &CalculateFileSizeThreadPool::sendFileSizeSignal, this,
                     &SidebarWidget::updateSiderbarFileSize);

    QObject::connect(DeviceListener::instance(), &DeviceListener::updateDevice, this,
                     &SidebarWidget::getUpdateDeviceSingla);

    // Update the UI interface of the sidebar for selecting file sizes.
    QObject::connect(UserSelectFileSize::instance(), &UserSelectFileSize::updateUserFileSelectSize,
                     this, &SidebarWidget::updateSelectSizeUi);

    QObject::connect(UserSelectFileSize::instance(), &UserSelectFileSize::updateUserFileSelectNum,
                     this, &SidebarWidget::updateCurSelectFileNum);

    QObject::connect(TransferHelper::instance(), &TransferHelper::remoteRemainSpace, this,
                     &SidebarWidget::updateRemoteSpaceSize);
    DLOG << "SidebarWidget init done";
}

SidebarWidget::~SidebarWidget()
{
    DLOG << "Destroying sidebar widget";
}

void SidebarWidget::updateUserSelectFileSizeUi()
{
    DLOG << "Updating selected file size UI";

    // allSizeStr = fromByteToQstring(allSize);
    QString text = QString("%1/%2").arg(selectSizeStr).arg(remoteSpaceSize);
    userSelectFileSize->setText(text);
}

QMap<QStandardItem *, DiskInfo> *SidebarWidget::getSidebarDiskList()
{
    return &sidebarDiskList;
}

void SidebarWidget::initSiderDataAndUi()
{
    DLOG << "Initializing sidebar data and UI";

    initSidebarSize();
    initSidebarUi();
}

void SidebarWidget::updateSiderDataAndUi(QStandardItem *siderbarItem, quint64 size)
{
    DLOG << "Updating sidebar data and UI Size:" << size;

    updateSidebarSize(siderbarItem, size);
    updateSiderbarUi(siderbarItem);
}

void SidebarWidget::updateSelectSizeUi(const QString &sizeStr)
{
    DLOG << "Updating selected size UI";
    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        DLOG << "Transfer method is LocalExport, updating local transfer label";
        localTransferLabel->setText(QString(tr("Select:%1").arg(sizeStr)));
    } else {
        DLOG << "Transfer method is NetworkTransmission, updating selected size label";
        selectSizeStr = sizeStr;
        updateUserSelectFileSizeUi();
        // update process
        updatePorcessLabel();
    }
}

void SidebarWidget::updateAllSizeUi(const quint64 &size, const bool &isAdd)
{
    DLOG << "Updating all size UI";
    if (isAdd) {
        DLOG << "Adding size to all size";
        allSize += size;
    } else {
        DLOG << "Subtracting size from all size";
        allSize -= size;
    }
    updateUserSelectFileSizeUi();
}

void SidebarWidget::addDiskFileNum(QStandardItem *siderbarItem, int num)
{
    DLOG << "Updating file number for sidebar item";
    sidebarDiskList[siderbarItem].allFileNum = num;
}

void SidebarWidget::changeUI()
{
    DLOG << "Changing UI based on transfer method";

    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        DLOG << "Transfer method is LocalExport, showing local transfer label";
        localTransferLabel->setVisible(true);
        userSelectFileSize->setVisible(false);
        processLabel->setVisible(false);
    } else if (method == TransferMethod::kNetworkTransmission) {
        DLOG << "Transfer method is NetworkTransmission, showing network transfer labels";
        localTransferLabel->setVisible(false);
        userSelectFileSize->setVisible(true);
        processLabel->setVisible(true);
    } else {
        DLOG << "Unknown transfer method:" << method.toStdString();
    }
}

void SidebarWidget::updateSiderbarFileSize(quint64 fileSize, const QString &path)
{
    DLOG << "Updating sidebar file size - Path:" << path.toStdString()
             << "Size:" << fileSize;

    QString rootPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString cPath = QDir(rootPath).rootPath();

    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();

    if (!path.contains(cPath)) {
        //    LOG << path << "is not c" << QFileInfo(path).absolutePath() << " " << cPath;
        return;
    }

    QStandardItem *item = (*filemap)[path].siderbarItem;
    updateSiderDataAndUi(item, fileSize);

    // add allsize
    updateAllSizeUi(fileSize, true);
}

void SidebarWidget::getUpdateDeviceSingla()
{
    DLOG << "Processing device update signal";

    QString rootPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString cPath = QDir(rootPath).rootPath();

    QList<QStorageInfo> devices = QStorageInfo::mountedVolumes();

    for (const QStorageInfo &device : devices) {
        if (device.isReadOnly() || !device.isReady()) {
            DLOG << "Excluding read-only or unready device:" << device.rootPath().toStdString();
            devices.removeOne(device);
            continue;
        }
        QString rootPath = device.rootPath();

        // delete C://
        if (rootPath.contains(cPath)) {
            DLOG << "Excluding C: drive:" << rootPath.toStdString();
            devices.removeOne(device);
            continue;
        }

        if (deviceList.contains(device)) {
            DLOG << "Device already in list, skipping:" << device.rootPath().toStdString();
            deviceList.removeOne(device);
            continue;
        }
        // add device
        updateDevice(device, true);
    }

    for (const QStorageInfo &device : deviceList) {
        // del device
        DLOG << "Removing device:" << device.rootPath().toStdString();
        updateDevice(device, false);
    }
    deviceList = devices;
}

void SidebarWidget::updateCurSelectFileNum(const QString &path, bool isAdd)
{
    DLOG << "Updating current selected file number";
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    QStandardItem *siderbarItem = filemap->value(path).siderbarItem;
    if (isAdd) {
        sidebarDiskList[siderbarItem].curSelectFileNum++;
    } else {
        sidebarDiskList[siderbarItem].curSelectFileNum--;
    }

    updateSelectFileNumState(siderbarItem);
}

void SidebarWidget::onClick(const QModelIndex &index)
{
    DLOG << "Sidebar item clicked";
    if (model()->data(index, Qt::WhatsThisRole).toBool()) {
        model()->setData(index, false, Qt::WhatsThisRole);
        QString path = model()->data(index, Qt::UserRole).toString();
        for (auto iterator = sidebarDiskList.begin(); iterator != sidebarDiskList.end();
             ++iterator) {
            QString rootPath = iterator.value().rootPath;
            if (path == rootPath) {
                emit selectOrDelAllFileViewItem(iterator.key());
            }
        }
    }
    for (int row = 0; row < model()->rowCount(); ++row) {
        QModelIndex itemIndex = model()->index(row, 0);
        if (itemIndex != index) {
            model()->setData(itemIndex, Qt::Unchecked, Qt::CheckStateRole);
        }
    }
}

void SidebarWidget::updateRemoteSpaceSize(int size)
{
    DLOG << "Updating remote space size";
    if (size < 1024) {
        DLOG << "Size is less than 1024, updating as GB";
        remoteSpaceSize = QString("%1GB").arg(size);
    } else {
        DLOG << "Size is greater than 1024, updating as TB";
        remoteSpaceSize = QString("%1TB").arg(float(size / 1024));
    }
    updateUserSelectFileSizeUi();
}

void SidebarWidget::updateDevice(const QStorageInfo &device, const bool &isAdd)
{
    DLOG << "[SidebarWidget]" << (isAdd ? "Adding" : "Removing");

    if (isAdd) {
        // add ui
        DLOG << "Adding device UI";
        QString rootPath = device.rootPath();
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(this->model());
        QStandardItem *item = new QStandardItem();
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        QString displayName = (device.name().isEmpty() ? tr("local disk") : device.name()) + "("
                + rootPath.at(0) + ":)";
        item->setData(displayName, Qt::DisplayRole);
        item->setData(rootPath, Qt::UserRole);
        item->setData(2, Qt::StatusTipRole);
        quint64 size = device.bytesTotal() - device.bytesAvailable();
        item->setData(fromByteToQstring(size), Qt::ToolTipRole);
        model->appendRow(item);
        //  LOG << " model->appendRow:" << rootPath << "model.size:" << model->rowCount();

        // add device in sidebar
        DiskInfo siderbarDiskInfo;
        siderbarDiskInfo.size = size;
        siderbarDiskInfo.rootPath = rootPath;
        sidebarDiskList[item] = siderbarDiskInfo;

        updateAllSizeUi(size, true);
        // update ui
        LOG << "updateDevice add updatePorcessLabel";
        updatePorcessLabel();

        emit updateFileview(item, true);
    } else {
        // del ui
        DLOG << "Deleting device UI";
        QString rootPath = device.rootPath();
        LOG << "del rootPath" << rootPath.toStdString();
        auto iterator = sidebarDiskList.begin();
        while (iterator != sidebarDiskList.end()) {

            if (rootPath == iterator.value().rootPath) {
                QStandardItem *item = iterator.key();
                quint64 size = device.bytesTotal() - device.bytesAvailable();
                UserSelectFileSize::instance()->delDevice(item);
                updateAllSizeUi(size, false);
                // update ui
                updatePorcessLabel();
                // update fileview
                emit updateFileview(item, false);
                // del file info from filemap
                CalculateFileSizeThreadPool::instance()->delDevice(item);

                // del device in sidebar
                sidebarDiskList.erase(iterator);

                for (int row = 0; row < this->model()->rowCount(); ++row) {
                    QModelIndex index = this->model()->index(row, 0);
                    QString path = this->model()->data(index, Qt::UserRole).toString();
                    if (rootPath == path) {
                        this->model()->removeRow(row);
                        break;
                    }
                }
                // delete item;
            }

            ++iterator;
        }
    }
}

void SidebarWidget::paintEvent(QPaintEvent *event)
{
    QListView::paintEvent(event);
    QPainter painter(viewport());
    painter.setPen(QPen(QColor(0, 0, 0, 200), 0.3));
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.moveTo(10, 216);
    path.lineTo(190, 216);
    painter.drawPath(path);
}

void SidebarWidget::initData()
{
    DLOG << "Initializing sidebar data";

    setItemDelegate(new SidebarItemDelegate());
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    QStandardItemModel *model = new QStandardItemModel(this);
    setModel(model);

    // user dir
    for (auto iterator = UserPath.begin(); iterator != UserPath.end(); ++iterator) {
        QStandardItem *item = new QStandardItem();
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        item->setData(iterator.key(), Qt::DisplayRole);
        item->setData(iterator.value(), Qt::UserRole);
        item->setData(2, Qt::StatusTipRole);
        model->appendRow(item);

        // add device in sidebar
        DiskInfo siderbarDiskInfo;
        siderbarDiskInfo.size = 0;
        siderbarDiskInfo.rootPath = iterator.value();
        sidebarDiskList[item] = siderbarDiskInfo;
        DLOG << "Added user path:" << iterator.key().toStdString() << "with root path:" << iterator.value().toStdString();
    }

    LOG << "sider model init size:" << model->rowCount();
    QObject::connect(this, &QListView::clicked, this, &SidebarWidget::onClick);
}

void SidebarWidget::initUi()
{
    DLOG << "Initializing sidebar UI";

    userSelectFileSize = new QLabel(this);
    userSelectFileSize->setStyleSheet(".QLabel{opacity: 1;"
                                      "color: rgba(82,106,127,1);"
                                      "font-family: \"SourceHanSansSC - Normal\";"
                                      "font-size: 12px;"
                                      "font-weight: 400;"
                                      "font-style: normal;"
                                      "text-align: left;}");
    userSelectFileSize->setText("0/0B");
    userSelectFileSize->setGeometry(70, 460, 100, 17);
    processLabel = new ProgressBarLabel(this);
    processLabel->setFixedSize(120, 8);
    processLabel->setGeometry(40, 480, processLabel->width(), processLabel->height());
    processLabel->setVisible(false);
    userSelectFileSize->setVisible(false);

    localTransferLabel = new QLabel(this);
    localTransferLabel->setStyleSheet(".QLabel{opacity: 1;"
                                      "color: rgba(82,106,127,1);"
                                      "font-family: \"SourceHanSansSC - Normal\";"
                                      "font-size: 14px;"
                                      "font-weight: 400;"
                                      "font-style: normal;"
                                      "text-align: left;}");
    localTransferLabel->setText(QString(tr("Select:%1").arg("0B")));
    localTransferLabel->setGeometry(70, 460, 100, 17);
    localTransferLabel->setVisible(false);
    DLOG << "Sidebar UI initialized";
}

void SidebarWidget::initSidebarSize()
{
    DLOG << "Initializing sidebar sizes";

    QString rootPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString cPath = QDir(rootPath).rootPath();

    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    for (auto iterator = filemap->begin(); iterator != filemap->end(); ++iterator) {
        if (iterator.value().isCalculate) {
            if (!QDir(iterator.key()).rootPath().contains(cPath)) {
                // DLOG << "File is not in C: drive, skipping:" << iterator.key().toStdString();
                continue;
            }
            sidebarDiskList[iterator.value().siderbarItem].size += iterator.value().size;
        }
    }
}

void SidebarWidget::initSidebarUi()
{
    DLOG << "Initializing sidebar UI components";

    quint64 tempSize = 0;
    for (auto iterator = sidebarDiskList.begin(); iterator != sidebarDiskList.end(); ++iterator) {
        iterator.key()->setData(fromByteToQstring(iterator.value().size), Qt::ToolTipRole);
        tempSize += iterator.value().size;
    }
    // update allsize ui
    updateAllSizeUi(tempSize, true);
}

void SidebarWidget::updateSiderbarUi(QStandardItem *siderbarItem)
{
    DLOG << "Updating sidebar item UI";
    QString sizeStr = fromByteToQstring(sidebarDiskList.value(siderbarItem).size);
    siderbarItem->setData(sizeStr, Qt::ToolTipRole);
}

void SidebarWidget::updatePorcessLabel()
{
    DLOG << "Updating progress label";

    quint64 selectSize = fromQstringToByte(selectSizeStr);
    double percentage =
            (static_cast<double>(selectSize) / fromQstringToByte(remoteSpaceSize)) * 100.0;
    int percentageAsInt = qBound(0, static_cast<int>(percentage), 100);
    //    LOG << "setProgress:" << percentageAsInt << "selectSize" << selectSize
    //            << "allSize:" << allSize;
    processLabel->setProgress(percentageAsInt);
}

void SidebarWidget::updateSelectFileNumState(QStandardItem *siderbarItem)
{
    DLOG << "Updating selection state for item";

    int curNum = sidebarDiskList.value(siderbarItem).curSelectFileNum;
    ListSelectionState state = ListSelectionState::unselected;
    if (curNum == 0) {
        state = ListSelectionState::unselected;
        DLOG << "Current selected item number is 0, state: unselected";
    } else if (curNum < sidebarDiskList.value(siderbarItem).allFileNum) {
        state = ListSelectionState::selecthalf;
        DLOG << "Current selected item number is less than total, state: selecthalf";
    } else {
        state = ListSelectionState::selectall;
        DLOG << "Current selected item number is equal to total, state: selectall";
    }
    // LOG << "root path:" << sidebarDiskList[siderbarItem].rootPath
    //        << " current :" << sidebarDiskList[siderbarItem].curSelectFileNum << "state" << state
    //       << sidebarDiskList[siderbarItem].allFileNum;
    int curState = siderbarItem->data(Qt::StatusTipRole).toInt();
    if (curState == state)
        return;
    emit updateSelectBtnState(siderbarItem, state);
    siderbarItem->setData(state, Qt::StatusTipRole);
}

void SidebarWidget::updateSidebarSize(QStandardItem *siderbarItem, quint64 size)
{
    DLOG << "Updating sidebar size for item";
    sidebarDiskList[siderbarItem].size += size;
}
