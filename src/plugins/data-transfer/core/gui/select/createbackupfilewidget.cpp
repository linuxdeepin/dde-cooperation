#include "createbackupfilewidget.h"

#include <QColor>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QToolButton>
#include <QLineEdit>
#include <QStackedWidget>
#include <QListView>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QStorageInfo>

#include <gui/connect/choosewidget.h>
#include <gui/mainwindow_p.h>

#pragma execution_character_set("utf-8")

CreateBackupFileWidget::CreateBackupFileWidget(QWidget *parent) : QFrame(parent)
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    QLabel *titileLabel = new QLabel("创建备份文件", this);
    titileLabel->setFixedHeight(30);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *fileNameLabel = new QLabel("文件信息", this);
    fileNameLabel->setStyleSheet("opacity: 1;"
                                 "color: rgba(0,26,46,1);"
                                 "font-family: \"SourceHanSansSC-Bold\";"
                                 "font-size: 16px;font-weight: 700;"
                                 "font-style: normal;"
                                 "letter-spacing: 0px;"
                                 "text-align: left;");
    QHBoxLayout *fileNameLayout = new QHBoxLayout(this);
    fileNameLayout->addSpacing(140);
    fileNameLayout->addWidget(fileNameLabel);
    fileNameLayout->setAlignment(Qt::AlignBottom);

    QFrame *fileName = new QFrame(this);
    QHBoxLayout *fileNameEditLayout = new QHBoxLayout(fileName);
    fileName->setLayout(fileNameEditLayout);
    fileName->setFixedSize(459, 36);
    fileName->setStyleSheet("border-radius: 8px;"
                            "opacity: 1;"
                            "background-color: rgba(0,0,0, 0.08);");

    QLabel *fileNameInputLabel1 = new QLabel(QString("文件名:"), fileName);
    fileNameInputLabel1->setFixedWidth(56);
    fileNameInputLabel1->setStyleSheet("opacity: 1;"
                                       "background-color: rgba(0,0,0,0);"
                                       "color: rgba(65,77,104,1);"
                                       "font-family: \"SourceHanSansSC-Medium\";"
                                       "font-size: 14px;font-weight: 500;"
                                       "font-style: normal;"
                                       "letter-spacing: 0px;"
                                       "text-align: left;");

    QLineEdit *fileNameInput = new QLineEdit(fileName);
    fileNameInput->setFixedWidth(211);
    fileNameInput->setPlaceholderText("默认文件名");
    fileNameInput->setStyleSheet("opacity: 1;"
                                 "background-color: rgba(0,0,0,0);"
                                 "color: rgba(82,106,127,1);"
                                 "font-family:\"SourceHanSansSC-Normal\";"
                                 "font-size: 12px;"
                                 "font-weight: 400;"
                                 "font-style: normal;"
                                 "letter-spacing: 0px;"
                                 "text-align: left;"
                                 "background-image: url(\":/icon/edit.svg\");"
                                 "background-repeat: no-repeat;"
                                 "background-position: center;");

    QHBoxLayout *fileNameInputLabel1Layout = new QHBoxLayout(fileName);
    fileNameInputLabel1Layout->addSpacing(10);
    fileNameInputLabel1Layout->addWidget(fileNameInputLabel1);
    fileNameInputLabel1Layout->addSpacing(10);
    fileNameInputLabel1Layout->addWidget(fileNameInput);

    QLabel *fileNameInputLabel2 = new QLabel(QString("大小:  %1").arg(backupFileSize), fileName);
    fileNameInputLabel2->setStyleSheet("opacity: 1;"
                                       "background-color: rgba(0,0,0,0);"
                                       "color: rgba(65,77,104,1);"
                                       "font-family: \"SourceHanSansSC-Medium\";"
                                       "font-size: 14px;font-weight: 500;"
                                       "font-style: normal;"
                                       "letter-spacing: 0px;"
                                       "text-align: left;");
    QHBoxLayout *fileNameInputLabel2Layout = new QHBoxLayout(fileName);
    fileNameInputLabel2Layout->addSpacing(20);
    fileNameInputLabel2Layout->addWidget(fileNameInputLabel2);

    fileNameEditLayout->addLayout(fileNameInputLabel1Layout);
    fileNameEditLayout->addLayout(fileNameInputLabel2Layout);

    QHBoxLayout *layout1 = new QHBoxLayout(this);
    layout1->addSpacing(140);
    layout1->addWidget(fileName);
    layout1->addSpacing(140);
    layout1->setAlignment(Qt::AlignTop);

    QLabel *savePathLabel1 = new QLabel("保存位置", this);
    savePathLabel1->setStyleSheet("opacity: 1;"
                                  "color: rgba(0,26,46,1);"
                                  "font-family: \"SourceHanSansSC-Bold\";"
                                  "font-size: 16px;"
                                  "font-weight: 700;"
                                  "font-style: normal;"
                                  "letter-spacing: 0px;"
                                  "text-align: left;");

    QLabel *savePathLabel2 = new QLabel("(选择备份磁盘)", this);
    savePathLabel2->setStyleSheet("opacity: 1;"
                                  "color: rgba(82,106,127,1);"
                                  "font-family: \"SourceHanSansSC-Normal\";"
                                  "font-size: 12px; "
                                  "font-weight: 400; "
                                  "font-style: normal; "
                                  "letter-spacing: 0px; "
                                  "text-align: left; ");
    QHBoxLayout *savePathLayout = new QHBoxLayout(this);
    savePathLayout->setAlignment(Qt::AlignTop);
    savePathLayout->addSpacing(140);
    savePathLayout->addWidget(savePathLabel1);
    savePathLayout->addWidget(savePathLabel2);


    QListView *diskListView = new QListView(this);

    diskListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diskListView->setSpacing(10);
    diskListView->setStyleSheet("background-color: rgba(0,0,0, 0.03);"
                                "margin-left: 10px; "
                                "margin-right: 10px; "
                               );
   diskListView->setFixedSize(460, 179); // 设置列表视图的位置和大小
    QStandardItemModel *model = new QStandardItemModel(this);
    diskListView->setModel(model);

    QList<QStorageInfo> drives = QStorageInfo::mountedVolumes();
    // 添加文件项数据
    for (const QStorageInfo &drive : drives) {
        QString rootPath = drive.rootPath();
        QString displayName = (drive.name().isEmpty() ? "本地磁盘" : drive.name()) + "(" + rootPath.at(0) + ":)";
        QStandardItem *item = new QStandardItem(QIcon(":/icon/app.svg"), displayName);
        item->setData(QColor(Qt::lightGray), Qt::BackgroundRole);

        item->setSizeHint(QSize(380, 48));
        model->appendRow(item);
    }


    QHBoxLayout *diskListViewLayout = new QHBoxLayout(this);
    diskListViewLayout->setAlignment(Qt::AlignTop);
    diskListViewLayout->addSpacing(140);
    diskListViewLayout->addWidget(diskListView);


    QToolButton *determineButton = new QToolButton(this);
    determineButton->setText("开始备份");
    determineButton->setFixedSize(120, 35);
    determineButton->setStyleSheet("background-color: lightgray;");
    determineButton->setEnabled(false);
    QObject::connect(determineButton, &QToolButton::clicked, this, &CreateBackupFileWidget::nextPage);


    QToolButton *cancelButton = new QToolButton(this);
    cancelButton->setText("取消");
    cancelButton->setFixedSize(120, 35);
    cancelButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(cancelButton, &QToolButton::clicked, this, &CreateBackupFileWidget::backPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(determineButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(2, this);
    indelabel->setAlignment(Qt::AlignCenter);
    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addSpacing(30);
    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(fileNameLayout);
    mainLayout->addLayout(layout1);
    mainLayout->addLayout(savePathLayout);
    mainLayout->addLayout(diskListViewLayout);
    mainLayout->addSpacing(30);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(indexLayout);
    QObject::connect(diskListView, &QListView::clicked, this,[determineButton](){
        determineButton->setEnabled(true);
    });
}

CreateBackupFileWidget::~CreateBackupFileWidget() { }

void CreateBackupFileWidget::nextPage()
{
    // nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                   "nullptr";
    }
}
void CreateBackupFileWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(data_transfer_core::PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                   "nullptr";
    }
}
