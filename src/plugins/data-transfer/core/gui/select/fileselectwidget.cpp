#include "fileselectwidget.h"
#include "item.h"
#include "calculatefilesize.h"

#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTreeView>
#include <QStandardItemModel>
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

const QList<QString> directories = {
    QString::fromLocal8Bit(Directory::kMovie),
    QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
    QString::fromLocal8Bit(Directory::kPicture),
    QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
    QString::fromLocal8Bit(Directory::kDocuments),
    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
    QString::fromLocal8Bit(Directory::kDownload),
    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
    QString::fromLocal8Bit(Directory::kMusic),
    QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
    QString::fromLocal8Bit(Directory::kDesktop),
    QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
};

static inline constexpr char InternetText[]{ "请选择要同步的文件" };
static inline constexpr char LocalText[]{ "请选择要备份的文件" };

FileSelectWidget::FileSelectWidget(QListView *siderbarWidget, QWidget *parent)
    : sidebar(siderbarWidget), QFrame(parent)
{
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
    QObject::connect(determineButton, &QToolButton::clicked, this,
                     [this]() { emit isOk(SelectItemName::FILES, true); });

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
    QObject::connect(cancelButton, &QToolButton::clicked, this,
                     [this]() { emit isOk(SelectItemName::FILES, false); });

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

    QObject::connect(sidebar, &QListView::clicked, this, &FileSelectWidget::updateFileView);
    QObject::connect(titlebar, &ItemTitlebar::selectAll, this,
                     &FileSelectWidget::selectOrDelAllItem);
}

void FileSelectWidget::createFilesizeListen(QListView *listView)
{
    //    CalculateFileSizeListen *filesizeListen = new CalculateFileSizeListen(this);
    //    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
    //    for (int row = 0; row < model->rowCount(); ++row) {
    //        QModelIndex index = model->index(row, 0);
    //        filesizeListen->addFileList(listView, index);
    //    }
    ////    QObject::connect(filesizeListen, &CalculateFileSizeListen::updateFileSize, this,
    ////                     [this](const QString &size) {
    ////            qInfo()<<"aaaaaa";
    ////        },Qt::QueuedConnection);
    //    filesizeListen->doWork();
}

void FileSelectWidget::initFileView()
{
    QList<QListView *> filelistviewlist;

    QStandardItemModel *siderbarModel = qobject_cast<QStandardItemModel *>(sidebar->model());
    for (int row = 0; row < siderbarModel->rowCount(); ++row) {
        QModelIndex siderItemIndex = siderbarModel->index(row, 0);
        QString path = siderbarModel->data(siderItemIndex, Qt::UserRole).toString();
        QString name = siderbarModel->data(siderItemIndex, Qt::DisplayRole).toString();
        SelectListView *view = getFileView(path);
        fileViewList[name] = row;
        stackedWidget->insertWidget(row, view);
        QStandardItemModel *siderbarModel = qobject_cast<QStandardItemModel *>(sidebar->model());
        for (int row = 0; row < siderbarModel->rowCount(); ++row) {
            QModelIndex siderItemIndex = siderbarModel->index(row, 0);
            QString path = siderbarModel->data(siderItemIndex, Qt::UserRole).toString();
            QString name = siderbarModel->data(siderItemIndex, Qt::DisplayRole).toString();
            SelectListView *view = getFileView(path);
            fileViewList[name] = row;
            stackedWidget->insertWidget(row, view);
            //        if (siderbarModel->data(siderItemIndex, Qt::ToolTipRole).toString() == "") {
            //            createFilesizeListen(view);
            //        }

            filelistviewlist.push_back(view);
        }

        CalculateFileSizeThreadPool::instance()->init(filelistviewlist);
        QObject::connect(CalculateFileSizeThreadPool::instance(),
                         &CalculateFileSizeThreadPool::sendFileSizeSignal, this,
                         &FileSelectWidget::updateFilesize);
        stackedWidget->setCurrentIndex(0);
    }
}
void FileSelectWidget::updateFileView(const QModelIndex &index)
{
    QStandardItemModel *siderbarModel = qobject_cast<QStandardItemModel *>(sidebar->model());
    QString name = siderbarModel->data(index, Qt::DisplayRole).toString();
    stackedWidget->setCurrentIndex(fileViewList[name]);
}

void FileSelectWidget::selectOrDelAllItem()
{
    SelectListView *listview = qobject_cast<SelectListView *>(stackedWidget->currentWidget());
    listview->selectorDelAllItem();
}

void FileSelectWidget::updateFilesize(qlonglong fileSize, QListView *listview, QModelIndex index)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listview->model());
    model->setData(index, fromByteToQstring(fileSize), Qt::ToolTipRole);
}

// void FileSelectWidget::updateSideFilesize(qlonglong fileSize, QModelIndex index)
//{
////    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(sidebar->model());
////    model->setData(index, fromByteToQstring(fileSize), Qt::ToolTipRole);
//}

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
    QStringList selectFileLsit;
    QStringList selectFileSize;
    qint64 size = 0;

    for (int i = 0; i < stackedWidget->count(); ++i) {
        QListView *fileListView = qobject_cast<QListView *>(stackedWidget->widget(i));
        QStandardItemModel *fileListViewModel =
                qobject_cast<QStandardItemModel *>(fileListView->model());
        for (int row = 0; row < fileListViewModel->rowCount(); ++row) {
            QModelIndex fileItemIndex = fileListViewModel->index(row, 0);
            if (fileItemIndex.data(Qt::CheckStateRole) == Qt::Checked) {
                selectFileLsit.append(fileItemIndex.data(Qt::UserRole).toString());
                QString indexSize = fileItemIndex.data(Qt::ToolTipRole).toString();
                if (indexSize != "") {
                    size += fromQstringToByte(indexSize);
                }
            }
        }
    }
    selectFileSize.push_back(fromByteToQstring(size));
    OptionsManager::instance()->addUserOption(Options::kFile, selectFileLsit);
    OptionsManager::instance()->addUserOption(Options::KSelectFileSize, selectFileSize);
    qInfo() << "select file:" << selectFileLsit;
    qInfo() << "select file size:" << selectFileSize;
}

SelectListView *FileSelectWidget::getFileView(const QString &path)
{
    QFileInfoList fileinfos = QDir(path).entryInfoList();
    if (fileinfos.size() >= 2) {
        // remove Current directory and parent directory
        fileinfos.removeAt(0);
        fileinfos.removeAt(0);
    }

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
        // del Users
        if (fileinfos[i].fileName() == "Users")
            continue;
        ListItem *item = new ListItem();
        item->setData(fileinfos[i].fileName(), Qt::DisplayRole);
        item->setData("", Qt::ToolTipRole);
        item->setData(fileinfos[i].filePath(), Qt::UserRole);
        item->setIcon(QIcon(":/icon/folder.svg"));
        item->setCheckable(true);
        model->appendRow(item);
    }
    for (int i = 0; i < fileinfos.count(); i++) {
        if (fileinfos[i].isDir())
            continue;
        ListItem *item = new ListItem();
        item->setData(fileinfos[i].fileName(), Qt::DisplayRole);
        item->setData(fromByteToQstring(fileinfos[i].size()), Qt::ToolTipRole);
        item->setIcon(QIcon(":/icon/fileicon.svg"));
        item->setData(fileinfos[i].filePath(), Qt::UserRole);
        item->setCheckable(true);
        model->appendRow(item);
    }

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
}

SidebarWidget::~SidebarWidget() { }

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
    int row = 0;
    for (int i = 0; i < 6; i++) {
        QStandardItem *item = new QStandardItem();
        item->setCheckable(true);
        item->setCheckState(Qt::Unchecked);
        item->setData(directories[row], Qt::DisplayRole);
        item->setData(directories[row + 1], Qt::UserRole);
        model->appendRow(item);
        row += 2;
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
