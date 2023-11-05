#include "fileselectwidget.h"
#include "item.h"
#include "../transfer/transferringwidget.h"
#include "calculatefilesize.h"
#include "userselectfilesize.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTreeView>

#include <QStandardPaths>
#include <QUrl>
#include <QDir>
#include <QScrollBar>
#include <QHeaderView>
#include <QStorageInfo>
#include <QPainterPath>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
#include <gui/mainwindow_p.h>

#pragma execution_character_set("utf-8")

namespace Directory {
inline constexpr char kMovie[]{ "视频" };
inline constexpr char kPicture[]{ "图片" };
inline constexpr char kMusic[]{ "音乐" };
inline constexpr char kDocuments[]{ "文档" };
inline constexpr char kDownload[]{ "下载" };
inline constexpr char kDesktop[]{ "桌面" };
} // namespace Directory

const QMap<QString, QString> userPath{
    { Directory::kMovie, QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) },
    { Directory::kPicture, QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) },
    { Directory::kDocuments, QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) },
    { Directory::kDownload, QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) },
    { Directory::kMusic, QStandardPaths::writableLocation(QStandardPaths::MusicLocation) },
    { Directory::kDesktop, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) }
};
static inline constexpr char InternetText[]{ "请选择要同步的文件" };
static inline constexpr char LocalText[]{ "请选择要备份的文件" };

FileSelectWidget::FileSelectWidget(SidebarWidget *siderbarWidget, QWidget *parent)
    : sidebar(siderbarWidget), QFrame(parent)
{
    sidebarSizeList = sidebar->getSidebarSizeList();
    initUI();
}

FileSelectWidget::~FileSelectWidget() { }

void FileSelectWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    titileLabel = new QLabel(LocalText, this);
    titileLabel->setFixedHeight(30);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    ItemTitlebar *titlebar =
            new ItemTitlebar("文件名", "大小", 50, 360, QRectF(10, 12, 16, 16), 3, this);
    titlebar->setFixedSize(500, 36);
    headerLayout->addWidget(titlebar);

    QHBoxLayout *fileviewLayout = new QHBoxLayout();
    stackedWidget = new QStackedWidget(this);
    initFileView();
    fileviewLayout->addWidget(stackedWidget);

    QLabel *tipLabel1 = new QLabel("传输完成的数据，将被存放在用户的 home 目录下", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel1->setFixedHeight(30);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    QToolButton *determineButton = new QToolButton(this);
    QPalette palette = determineButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    determineButton->setPalette(palette);
    determineButton->setText("确定");
    determineButton->setFixedSize(120, 35);
    determineButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                   "border: 1px solid rgba(0,0,0, 0.03);"
                                   "opacity: 1;"
                                   "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                                   "rgba(37, 183, 255, 1), stop:1 rgba(0, 152, 255, 1));"
                                   "font-family: \"SourceHanSansSC-Medium\";"
                                   "font-size: 14px;"
                                   "font-weight: 500;"
                                   "color: rgba(255,255,255,1);"
                                   "font-style: normal;"
                                   "letter-spacing: 3px;"
                                   "text-align: center;"
                                   "}");
    QObject::connect(determineButton, &QToolButton::clicked, this, &FileSelectWidget::nextPage);

    QToolButton *cancelButton = new QToolButton(this);
    cancelButton->setText("取消");
    cancelButton->setFixedSize(120, 35);
    cancelButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                "border: 1px solid rgba(0,0,0, 0.03);"
                                "opacity: 1;"
                                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                                "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                                "font-family: \"SourceHanSansSC-Medium\";"
                                "font-size: 14px;"
                                "font-weight: 500;"
                                "color: rgba(65,77,104,1);"
                                "font-style: normal;"
                                "letter-spacing: 3px;"
                                "text-align: center;"
                                ";}");
    QObject::connect(cancelButton, &QToolButton::clicked, this, &FileSelectWidget::backPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(determineButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    mainLayout->addSpacing(30);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(headerLayout);

    mainLayout->addLayout(fileviewLayout);
    mainLayout->addSpacing(5);
    mainLayout->addLayout(buttonLayout);

    QObject::connect(sidebar, &QListView::clicked, this, &FileSelectWidget::changeFileView);
    QObject::connect(titlebar, &ItemTitlebar::selectAll, this,
                     &FileSelectWidget::selectOrDelAllItem);
}

void FileSelectWidget::startCalcluateFileSize()
{
    QList<QString> fileList;
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    for (auto iterator = filemap->begin(); iterator != filemap->end(); ++iterator) {
        if (!iterator.value().isCalculate) {
            fileList.push_back(iterator.key());
        }
    }
    CalculateFileSizeThreadPool::instance()->work(fileList);

    // update fileview file size
    QObject::connect(CalculateFileSizeThreadPool::instance(),
                     &CalculateFileSizeThreadPool::sendFileSizeSignal, this,
                     &FileSelectWidget::updateFileViewSize);

    // Update the size of the selected files to be computed.
    QObject::connect(CalculateFileSizeThreadPool::instance(),
                     &CalculateFileSizeThreadPool::sendFileSizeSignal,
                     UserSelectFileSize::instance(), &UserSelectFileSize::updatependingFileSize);
    // Update the UI interface of the sidebar for selecting file sizes.
    QObject::connect(UserSelectFileSize::instance(), &UserSelectFileSize::updateUserFileSelectSize,
                     sidebar, &SidebarWidget::updateSelectSizeUi);
}

void FileSelectWidget::initFileView()
{
    QStandardItemModel *siderbarModel = qobject_cast<QStandardItemModel *>(sidebar->model());
    for (int row = 0; row < siderbarModel->rowCount(); ++row) {
        QModelIndex siderItemIndex = siderbarModel->index(row, 0);
        QString path = siderbarModel->data(siderItemIndex, Qt::UserRole).toString();
        (*sidebarSizeList)[siderItemIndex] = 0;
        SelectListView *view = initFileView(path, siderItemIndex);
        sidebarFileViewList[siderItemIndex] = view;
        stackedWidget->insertWidget(row, view);
    }
    // init sidebar user directory size
    sidebar->initSiderDataAndUi();

    stackedWidget->setCurrentIndex(0);
    startCalcluateFileSize();
}

void FileSelectWidget::changeFileView(const QModelIndex &index)
{
    stackedWidget->setCurrentWidget(sidebarFileViewList[index]);
}

void FileSelectWidget::selectOrDelAllItem()
{
    SelectListView *listview = qobject_cast<SelectListView *>(stackedWidget->currentWidget());
    listview->selectorDelAllItem();
}

void FileSelectWidget::updateFileViewSize(quint64 fileSize, const QString &path)
{
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    if (filemap->contains(path)) {
        QStandardItem *item = (*filemap)[path].fileItem;
        item->setData(fromByteToQstring(fileSize), Qt::ToolTipRole);
    }
}

void FileSelectWidget::updateFileSelectList(QStandardItem *item)
{
    QString path = item->data(Qt::UserRole).toString();
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    if (item->data(Qt::CheckStateRole) == Qt::Unchecked) {
        if ((*filemap)[path].isSelect == false) {
            return;
        }
        // do not select the file
        (*filemap)[path].isSelect = false;
        UserSelectFileSize::instance()->delSelectFiles(path);
        if ((*filemap)[path].isCalculate) {
            quint64 size = (*filemap)[path].size;
            UserSelectFileSize::instance()->delUserSelectFileSize(size);
        } else {
            UserSelectFileSize::instance()->delPendingFiles(path);
        }
    } else if (item->data(Qt::CheckStateRole) == Qt::Checked) {
        if ((*filemap)[path].isSelect == true) {
            return;
        }
        (*filemap)[path].isSelect = true;
        UserSelectFileSize::instance()->addSelectFiles(path);
        if ((*filemap)[path].isCalculate) {
            quint64 size = (*filemap)[path].size;
            UserSelectFileSize::instance()->addUserSelectFileSize(size);
        } else {
            UserSelectFileSize::instance()->addPendingFiles(path);
        }
    }
}

void FileSelectWidget::nextPage()
{
    // send useroptions
    sendOptions();

    // nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void FileSelectWidget::backPage()
{
    // delete Options
    delOptions();

    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void FileSelectWidget::sendOptions()
{
    QStringList selectFileLsit = UserSelectFileSize::instance()->getSelectFilesList();
    OptionsManager::instance()->addUserOption(Options::kFile, selectFileLsit);
    qInfo() << "select file:" << selectFileLsit;

    emit isOk(SelectItemName::FILES);
}

void FileSelectWidget::delOptions()
{
    // Clear All File Selections
    QStringList filelist = UserSelectFileSize::instance()->getSelectFilesList();
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    for (QString filePath : filelist) {
        QStandardItem *item = (*filemap)[filePath].fileItem;
        item->setCheckState(Qt::Unchecked);
    }
    OptionsManager::instance()->addUserOption(Options::kFile, QStringList());
    emit isOk(SelectItemName::FILES);
}

SelectListView *FileSelectWidget::initFileView(const QString &path,
                                               const QModelIndex &siderbarIndex)
{
    QDir directory(path);
    // del . and ..
    directory.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList fileinfos = directory.entryInfoList();

    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    ItemDelegate *delegate = new ItemDelegate(99, 250, 379, 100, 50, QPoint(65, 6), QPoint(10, 9));

    QStandardItemModel *model = new QStandardItemModel(this);
    SelectListView *fileView = new SelectListView(this);
    fileView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileView->setModel(model);
    fileView->setItemDelegate(delegate);
    fileView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fileView->setSelectionMode(QAbstractItemView::NoSelection);
    for (int i = 0; i < fileinfos.count(); i++) {
        if (!fileinfos[i].isDir())
            continue;
        if (fileinfos[i].fileName() == "Users")
            continue;
        QStandardItem *item = new QStandardItem();
        item->setData(fileinfos[i].fileName(), Qt::DisplayRole);
        item->setData("", Qt::ToolTipRole);
        item->setData(fileinfos[i].filePath(), Qt::UserRole);
        item->setIcon(QIcon(":/icon/folder.svg"));
        item->setCheckable(true);
        model->appendRow(item);

        // add fileInfo
        FileInfo fileInfo;
        fileInfo.size = 0;
        fileInfo.isCalculate = false;
        fileInfo.isSelect = false;
        fileInfo.fileItem = item;
        fileInfo.siderIndex = siderbarIndex;
        (*filemap)[fileinfos[i].filePath()] = fileInfo;
    }
    for (int i = 0; i < fileinfos.count(); i++) {
        if (!fileinfos[i].isFile())
            continue;
        if (fileinfos[i].isSymLink())
            continue;
        QStandardItem *item = new QStandardItem();
        item->setData(fileinfos[i].fileName(), Qt::DisplayRole);
        item->setData(fromByteToQstring(fileinfos[i].size()), Qt::ToolTipRole);
        item->setIcon(QIcon(":/icon/fileicon.svg"));
        item->setData(fileinfos[i].filePath(), Qt::UserRole);

        item->setCheckable(true);
        model->appendRow(item);

        // add fileInfo
        FileInfo fileInfo;
        fileInfo.size = fileinfos[i].size();
        fileInfo.isCalculate = true;
        fileInfo.isSelect = false;
        fileInfo.fileItem = item;
        fileInfo.siderIndex = siderbarIndex;
        (*filemap)[fileinfos[i].filePath()] = fileInfo;
    }

    QObject::connect(model, &QStandardItemModel::itemChanged, this,
                     &FileSelectWidget::updateFileSelectList);
    return fileView;
}

void FileSelectWidget::changeText()
{
    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        titileLabel->setText(LocalText);
    } else if (method == TransferMethod::kNetworkTransmission) {
        titileLabel->setText(InternetText);
    }
}

SidebarWidget::SidebarWidget(QWidget *parent) : QListView(parent)
{
    initData();
    initUi();

    // update siderbar file size
    QObject::connect(CalculateFileSizeThreadPool::instance(),
                     &CalculateFileSizeThreadPool::sendFileSizeSignal, this,
                     &SidebarWidget::updateSiderbarFileSize);
}

SidebarWidget::~SidebarWidget() { }

void SidebarWidget::updateUserSelectFileSizeUi()
{
    QString text = QString("%1/%2").arg(selectSizeStr).arg(allSizeStr);
    userSelectFileSize->setText(text);
}

QMap<QModelIndex, quint64> *SidebarWidget::getSidebarSizeList()
{
    return &sidebarSizeList;
}

void SidebarWidget::initSiderDataAndUi()
{
    initSiderbarSize();
    initSiderbarUi();
}

void SidebarWidget::updateSiderDataAndUi(QModelIndex index, quint64 size)
{
    updateSiderbarSize(index, size);
    updateSiderbarUi(index);
}

void SidebarWidget::updateSelectSizeUi(const QString &sizeStr)
{
    selectSizeStr = sizeStr;
    updateUserSelectFileSizeUi();

    // update process
    updatePorcessLabel();
}

void SidebarWidget::updateAllSizeUi()
{
    allSizeStr = fromByteToQstring(allSize);
    updateUserSelectFileSizeUi();
}

void SidebarWidget::updateSiderbarFileSize(quint64 fileSize, const QString &path)
{
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    if (filemap->contains(path)) {
        QModelIndex index = (*filemap)[path].siderIndex;
        updateSiderDataAndUi(index, fileSize);
    }
}

void SidebarWidget::paintEvent(QPaintEvent *event)
{
    QListView::paintEvent(event);
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(0, 0, 0, 50), 2));
    QPainterPath path;
    path.moveTo(10, 216);
    path.lineTo(190, 216);
    painter.drawPath(path);
}

void SidebarWidget::initData()
{
    setItemDelegate(new SidebarItemDelegate());
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    QStandardItemModel *model = new QStandardItemModel(this);
    setModel(model);

    // user dir
    for (auto iterator = userPath.begin(); iterator != userPath.end(); ++iterator) {
        QStandardItem *item = new QStandardItem();
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        item->setData(iterator.key(), Qt::DisplayRole);
        item->setData(iterator.value(), Qt::UserRole);
        model->appendRow(item);
    }
    // Storage dir
    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();

    for (const QStorageInfo &drive : drives) {
        QStandardItem *item = new QStandardItem();
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        QString rootPath = drive.rootPath();
        // delete C://
        if (rootPath.contains("C:")) {
            continue;
        }
        QString displayName =
                (drive.name().isEmpty() ? "本地磁盘" : drive.name()) + "(" + rootPath.at(0) + ":)";

        item->setData(displayName, Qt::DisplayRole);
        item->setData(rootPath, Qt::UserRole);
        item->setData(fromByteToQstring(drive.bytesTotal() - drive.bytesAvailable()),
                      Qt::ToolTipRole);
        model->appendRow(item);
    }

    QObject::connect(this, &QListView::clicked, [this, model](const QModelIndex &index) {
        for (int row = 0; row < model->rowCount(); ++row) {
            QModelIndex itemIndex = model->index(row, 0);
            if (itemIndex != index) {
                model->setData(itemIndex, Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    });
}

void SidebarWidget::initUi()
{
    userSelectFileSize = new QLabel(this);
    userSelectFileSize->setStyleSheet(".QLabel{opacity: 1;"
                                      "color: rgba(82,106,127,1);"
                                      "font-family: \"SourceHanSansSC - Normal\";"
                                      "font-size: 12px;"
                                      "font-weight: 400;"
                                      "font-style: normal;"
                                      "letter-spacing: 0px;"
                                      "text-align: left;}");
    userSelectFileSize->setText("0/0B");
    userSelectFileSize->setGeometry(70, 460, 100, 17);
    processLabel = new ProgressBarLabel(this);
    processLabel->setFixedSize(120, 8);
    processLabel->setGeometry(40, 480, processLabel->width(), processLabel->height());
}

void SidebarWidget::initSiderbarSize()
{
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    for (auto iterator = filemap->begin(); iterator != filemap->end(); ++iterator) {
        if (iterator.value().isCalculate) {
            sidebarSizeList[iterator.value().siderIndex] += iterator.value().size;
            // add allsize
            allSize += iterator.value().size;
        }
    }
}

void SidebarWidget::initSiderbarUi()
{
    QStandardItemModel *siderbarModel = qobject_cast<QStandardItemModel *>(this->model());
    for (auto iterator = sidebarSizeList.begin(); iterator != sidebarSizeList.end(); ++iterator) {
        siderbarModel->setData(iterator.key(), fromByteToQstring(iterator.value()),
                               Qt::ToolTipRole);
    }
    // update allsize ui
    updateAllSizeUi();
}

void SidebarWidget::updateSiderbarUi(QModelIndex index)
{
    QString sizeStr = fromByteToQstring(sidebarSizeList[index]);
    QStandardItemModel *siderbarModel = qobject_cast<QStandardItemModel *>(this->model());
    siderbarModel->setData(index, sizeStr, Qt::ToolTipRole);

    // update allsize ui
    updateAllSizeUi();
}

void SidebarWidget::updatePorcessLabel()
{
    quint64 selectSize = fromQstringToByte(selectSizeStr);
    quint64 allSize = fromQstringToByte(allSizeStr);
    double percentage = (static_cast<double>(selectSize) / allSize) * 100.0;
    int percentageAsInt = qBound(0, static_cast<int>(percentage), 100);

    processLabel->setProgress(percentageAsInt);
}

void SidebarWidget::updateSiderbarSize(QModelIndex index, quint64 size)
{
    sidebarSizeList[index] += size;
    // add allsize
    allSize += size;
}
