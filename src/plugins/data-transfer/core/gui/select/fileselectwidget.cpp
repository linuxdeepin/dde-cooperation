#include "fileselectwidget.h"
#include "item.h"
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

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
#include <gui/mainwindow_p.h>

#pragma execution_character_set("utf-8")

const QList<QString> directories = {
    QString::fromLocal8Bit(Directory::kMovie), QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
    QString::fromLocal8Bit(Directory::kPicture), QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
    QString::fromLocal8Bit(Directory::kDocuments), QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
    QString::fromLocal8Bit(Directory::kDownload), QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
    QString::fromLocal8Bit(Directory::kMusic), QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
    QString::fromLocal8Bit(Directory::kDesktop), QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
};

inline constexpr char internetMethodSelectFileName[] {"请选择要同步的文件"};
inline constexpr char localFileMethodSelectFileName[] {"请选择要备份的文件"};

FileSelectWidget::FileSelectWidget(QListView *siderbarWidget, QWidget *parent)
    : QFrame(parent)
{
    initSiderBar(siderbarWidget);
    initUI();
    initConnect(sidebar);
    initConnect(fileview);
}

FileSelectWidget::~FileSelectWidget()
{
}

void FileSelectWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    QLabel *titileLabel = new QLabel(internetMethodSelectFileName, this);
    titileLabel->setFixedHeight(30);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    ItemTitlebar *titlebar = new ItemTitlebar("文件名","大小",50,360,QRectF(10,12,16,16),3, this);
    titlebar->setFixedSize(500,36);
    headerLayout->addWidget(titlebar);
    initFileView();

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
    QObject::connect(determineButton,&QToolButton::clicked,this,[this](){
        emit isOk(SelectItemName::FILES,true);
    });


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
    QObject::connect(cancelButton,&QToolButton::clicked,this,[this](){
        emit isOk(SelectItemName::FILES,false);
    });

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
   // mainLayout->addWidget(titlebar);
    mainLayout->addWidget(fileview);
    mainLayout->addSpacing(5);
    mainLayout->addLayout(buttonLayout);
}

void FileSelectWidget::initConnect(QAbstractItemView *view)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(view->model());
    connect(model, &QStandardItemModel::itemChanged, [this](QStandardItem *changedItem) {
        if (!aync)
            return;
        QString filepath = changedItem->data(Qt::UserRole).toString();
        if (changedItem->checkState() == Qt::Checked) {
            if (!seletFileList.contains(filepath)) {
                seletFileList.append(filepath);
                qInfo() << filepath << "add";
            }
        } else {
            if (seletFileList.contains(filepath)) {
                seletFileList.removeOne(filepath);
                qInfo() << filepath << "del";
            }
        }
    });
}

void FileSelectWidget::initFileView()
{
    QStandardItemModel *model = new QStandardItemModel(this);
    fileview = new QListView(this);
    fileview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileview->setModel(model);
    fileview->setItemDelegate(new ItemDelegate(99,250,379,100,50,QPoint(65,6),QPoint(10,9)));
    fileview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fileview->setSelectionMode(QAbstractItemView::NoSelection);
    updateFileView();
}

void FileSelectWidget::updateFileView()
{
    if (!sidebar || sidebar->selectionModel()->selectedIndexes().isEmpty())
        return;

    QModelIndex index = sidebar->selectionModel()->selectedIndexes().first();
    QString itemText = sidebar->model()->data(index, Qt::DisplayRole).toString();

    QString path = sidebar->model()->data(index, Qt::UserRole).toString();

    QFileInfoList fileinfos = QDir(path).entryInfoList();
    if (fileinfos.size() >= 2) {
        // remove Current directory and parent directory
        fileinfos.removeAt(0);
        fileinfos.removeAt(0);
    }

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(fileview->model());
    if (!model)
        return;

    aync = false;
    model->clear();
    for (int i = 0; i < fileinfos.count(); i++) {
        if (!fileinfos[i].isDir())
            continue;

        ListItem *item = new ListItem();
        item->setData(fileinfos[i].fileName(), Qt::DisplayRole);
        item->setData("", Qt::ToolTipRole);
        item->setData(fileinfos[i].filePath(), Qt::UserRole);
        item->setIcon(QIcon(":/icon/folder.svg"));
        item->setCheckable(true);

        if (!seletFileList.contains(fileinfos[i].filePath()))
            item->setCheckState(Qt::Unchecked);
        else
            item->setCheckState(Qt::Checked);

        model->appendRow(item);
    }
    for (int i = 0; i < fileinfos.count(); i++) {
        if (fileinfos[i].isDir())
            continue;

        ListItem *item = new ListItem();
        item->setData(fileinfos[i].fileName(), Qt::DisplayRole);
        item->setData("1M", Qt::ToolTipRole);
        item->setIcon(QIcon(":/icon/fileicon.svg"));
        item->setData(fileinfos[i].filePath(), Qt::UserRole);
        item->setCheckable(true);

        if (!seletFileList.contains(fileinfos[i].filePath()))
            item->setCheckState(Qt::Unchecked);
        else
            item->setCheckState(Qt::Checked);

        model->appendRow(item);
    }
    aync = true;
}

void FileSelectWidget::initSiderBar(QListView *siderbarWidget)
{
    if (!siderbarWidget) {
        qWarning() << "Sidebar init failed";
        return;
    }
    sidebar = siderbarWidget;
    QWidget *parent = qobject_cast<QWidget *>(sidebar->parent());
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget && parent)
        connect(stackedWidget, &QStackedWidget::currentChanged, this, [this, stackedWidget, parent]() {
            if (stackedWidget->currentWidget() == this)
                parent->setVisible(true);
            else
                parent->setVisible(false);
        });
    connect(sidebar->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FileSelectWidget::updateFileView);
}

void FileSelectWidget::nextPage()
{
    //send useroptions
    sendOptions();

    //nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void FileSelectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void FileSelectWidget::update()
{
}

void FileSelectWidget::sendOptions()
{
    qInfo() << "select file :" << seletFileList;
    OptionsManager::instance()->addUserOption(Options::kFile, seletFileList);

    //transfer
    TransferHelper::instance()->startTransfer();
}

SidebarWidget::SidebarWidget(QWidget *parent)
    : QListView(parent)
{
    setItemDelegate(new SidebarItemDelegate());
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    initData();
}

SidebarWidget::~SidebarWidget()
{
}

void SidebarWidget::initData()
{
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
            QString displayName = (drive.name().isEmpty()?"本地磁盘":drive.name()) +"("+rootPath.at(0)+":)";


            item->setData(displayName, Qt::DisplayRole);
            item->setData(rootPath, Qt::UserRole);
            model->appendRow(item);
        }
}
