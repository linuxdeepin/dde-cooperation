#include "siderbarwidget.h"
#include "fileselectwidget.h"
#include "item.h"
#include "calculatefilesize.h"
#include "userselectfilesize.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QDir>

#include <utils/optionsmanager.h>

#pragma execution_character_set("utf-8")

static inline constexpr char InternetText[]{ "请选择要同步的文件" };
static inline constexpr char LocalText[]{ "请选择要备份的文件" };

FileSelectWidget::FileSelectWidget(SidebarWidget *siderbarWidget, QWidget *parent)
    : QFrame(parent), sidebar(siderbarWidget)
{
    initUI();

    QObject::connect(sidebar, &SidebarWidget::updateFileview, this,
                     &FileSelectWidget::updateFileViewData);

    // update fileview file size
    QObject::connect(CalculateFileSizeThreadPool::instance(),
                     &CalculateFileSizeThreadPool::sendFileSizeSignal, this,
                     &FileSelectWidget::updateFileViewSize);

    QObject::connect(sidebar, &QListView::clicked, this, &FileSelectWidget::changeFileView);
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

    QObject::connect(titlebar, &ItemTitlebar::selectAll, this,
                     &FileSelectWidget::selectOrDelAllItem);
    QObject::connect(sidebar, &SidebarWidget::selectOrDelAllFileViewItem, this,
                     &FileSelectWidget::selectOrDelAllItemFromSiderbar);
}

void FileSelectWidget::startCalcluateFileSize(QList<QString> fileList)
{
    CalculateFileSizeThreadPool::instance()->work(fileList);
}

void FileSelectWidget::initFileView()
{
    QMap<QStandardItem *, DiskInfo> *diskList = sidebar->getSidebarDiskList();
    for (auto iterator = diskList->begin(); iterator != diskList->end(); ++iterator) {
        QStandardItem *diskItem = iterator.key();
        DiskInfo diskInfo = iterator.value();
        QString path = diskInfo.rootPath;
        SelectListView *view = addFileViewData(path, diskItem);
        sidebarFileViewList[diskItem] = view;
        stackedWidget->addWidget(view);
    }
    // init sidebar user directory size
    sidebar->initSiderDataAndUi();

    stackedWidget->setCurrentIndex(0);
}

void FileSelectWidget::changeFileView(const QModelIndex &siderbarIndex)
{
    QString path = sidebar->model()->data(siderbarIndex, Qt::UserRole).toString();
    QMap<QStandardItem *, DiskInfo> *list = sidebar->getSidebarDiskList();
    for (auto iteraotr = list->begin(); iteraotr != list->end(); ++iteraotr) {
        QString rootPath = iteraotr.value().rootPath;
        if (path == rootPath) {
            QStandardItem *item = iteraotr.key();
            stackedWidget->setCurrentWidget(sidebarFileViewList[item]);
        }
    }
}

void FileSelectWidget::selectOrDelAllItem()
{
    SelectListView *listview = qobject_cast<SelectListView *>(stackedWidget->currentWidget());
    listview->selectorDelAllItem();
}

void FileSelectWidget::updateFileViewSize(quint64 fileSize, const QString &path)
{
    QMap<QString, FileInfo> *filemap = CalculateFileSizeThreadPool::instance()->getFileMap();
    QStandardItem *item = (*filemap)[path].fileItem;
    item->setData(fromByteToQstring(fileSize), Qt::ToolTipRole);
}

void FileSelectWidget::updateFileViewData(QStandardItem *siderbarItem, const bool &isAdd)
{
    // del fileview
    if (!isAdd) {
        SelectListView *view = qobject_cast<SelectListView *>(sidebarFileViewList[siderbarItem]);
        stackedWidget->setCurrentIndex(0);
        stackedWidget->removeWidget(view);
        sidebarFileViewList.remove(siderbarItem);
        delete view;
        qInfo() << "del device";

    } else {
        // add fileview
        QMap<QStandardItem *, DiskInfo> *diskList = sidebar->getSidebarDiskList();
        QString path = diskList->value(siderbarItem).rootPath;
        qInfo() << "updateFileViewData add ...." << path;
        SelectListView *view = addFileViewData(path, siderbarItem);
        sidebarFileViewList[siderbarItem] = view;
        stackedWidget->addWidget(view);
        qInfo() << "add device";
    }
}

void FileSelectWidget::selectOrDelAllItemFromSiderbar(QStandardItem *siderbarItem)
{
    SelectListView *listview =
            qobject_cast<SelectListView *>(sidebarFileViewList.value(siderbarItem));
    listview->selectorDelAllItem();
}

SelectListView *FileSelectWidget::addFileViewData(const QString &path, QStandardItem *sidebaritem)
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

    int diskFileNUM = 0;
    QList<QString> needCalculateFileList;
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
        fileInfo.siderbarItem = sidebaritem;
        (*filemap)[fileinfos[i].filePath()] = fileInfo;

        needCalculateFileList.push_back(fileinfos[i].filePath());

        ++diskFileNUM;
    }
    for (int i = 0; i < fileinfos.count(); i++) {
        if (!fileinfos[i].isFile() || fileinfos[i].isSymLink())
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
        fileInfo.siderbarItem = sidebaritem;
        (*filemap)[fileinfos[i].filePath()] = fileInfo;

        ++diskFileNUM;
    }

    // add disk file num
    sidebar->addDiskFileNum(sidebaritem, diskFileNUM);

    // calculate filelist
    startCalcluateFileSize(needCalculateFileList);

    QObject::connect(model, &QStandardItemModel::itemChanged, UserSelectFileSize::instance(),
                     &UserSelectFileSize::updateFileSelectList);

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
