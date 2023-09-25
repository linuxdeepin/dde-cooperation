#include "fileselectwidget.h"
#include "item.h"

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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    QLabel *titileLabel = new QLabel(internetMethodSelectFileName, this);
    titileLabel->setFixedHeight(30);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QHBoxLayout *headerLayout = new QHBoxLayout(this);
    QLabel *hedaerIcon = new QLabel(this);
    QLabel *filename = new QLabel("文件名", this);
    QLabel *hedaerIcon2 = new QLabel(this);
    QLabel *size = new QLabel("大小", this);

    headerLayout->addWidget(hedaerIcon);
    headerLayout->addWidget(filename);
    headerLayout->addWidget(hedaerIcon2);
    headerLayout->addWidget(size);

    initFileView();

    QLabel *tipLabel1 = new QLabel("传输完成的数据，将被存放在用户的 home 目录下", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel1->setFixedHeight(50);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    QToolButton *determineButton = new QToolButton(this);
    QPalette palette = determineButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    determineButton->setPalette(palette);
    determineButton->setText("确定");
    determineButton->setFixedSize(120, 35);
    determineButton->setStyleSheet("background-color: #0098FF;");
    QObject::connect(determineButton, &QToolButton::clicked, this, &FileSelectWidget::nextPage);

    QToolButton *cancelButton = new QToolButton(this);
    cancelButton->setText("取消");
    cancelButton->setFixedSize(120, 35);
    cancelButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(cancelButton, &QToolButton::clicked, this, &FileSelectWidget::backPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(determineButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    mainLayout->addSpacing(30);
    mainLayout->addWidget(titileLabel);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(fileview);
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
    fileview->setItemDelegate(new ItemDelegate());
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
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void FileSelectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
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
