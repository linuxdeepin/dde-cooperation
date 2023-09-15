#include "filetranswidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
#pragma execution_character_set("utf-8")

const QStringList directories = {
    Directory::kDocuments,
    Directory::kMusic,
    Directory::kPicture,
    Directory::kMovie,
    Directory::kDownload
};

FileTransWidget::FileTransWidget(QWidget *parent)
    : QFrame(parent)
{
    userData = TransferHelper::instance()->getUserDataSize();
    remainStorage = TransferHelper::instance()->getRemainStorage();
    initUI();
}

FileTransWidget::~FileTransWidget()
{
}

void FileTransWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("请确认接收文件", this);
    titileLabel->setFixedHeight(40);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QHBoxLayout *layout1 = new QHBoxLayout(this);
    selectFrame = new QFrame(this);
    layout1->addWidget(selectFrame, Qt::AlignCenter);
    initSelectFrame();

    QLabel *tipLabel1 = new QLabel("请选择需要传输的数据，数据将被存放在当前用\n户的home目录下", this);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    tipLabel1->setFixedHeight(80);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    QToolButton *nextButton = new QToolButton(this);
    nextButton->setText("确定");
    nextButton->setFixedSize(300, 35);
    nextButton->setStyleSheet("background-color: blue;");
    connect(nextButton, &QToolButton::clicked, this, &FileTransWidget::nextPage);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(nextButton, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(layout1);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addLayout(layout);
}

void FileTransWidget::initSelectFrame()
{
    QLabel *userLabel = new QLabel("windows user", this);
    QHBoxLayout *layout1 = new QHBoxLayout(this);
    layout1->addSpacing(50);
    layout1->addWidget(userLabel, Qt::AlignCenter);

    selectLayout = new QGridLayout(this);

    for (int i = 0; i < directories.count(); i++) {
        const QString dir = directories[i];
        const QString size = QString::number(userData[dir]);
        QCheckBox *checkBox = new QCheckBox(dir, selectFrame);
        QLabel *label = new QLabel(size + "G", selectFrame);

        connect(checkBox, &QCheckBox::stateChanged, this, &FileTransWidget::update);

        selectLayout->addWidget(checkBox, i, 0);
        selectLayout->addWidget(label, i, 1);
    }

    QHBoxLayout *layout2 = new QHBoxLayout(this);
    layout2->addSpacing(50);
    layout2->addLayout(selectLayout);

    QString tip = "数据盘可用空间%1G，已选数据共约11G";
    tip = tip.arg(remainStorage);
    storageInfoLabel = new QLabel(tip, this);
    storageInfoLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    QFont font;
    font.setPointSize(8);
    storageInfoLabel->setFont(font);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(layout1);
    mainLayout->addLayout(layout2);
    mainLayout->addWidget(storageInfoLabel);

    selectFrame->setLayout(mainLayout);
    selectFrame->setStyleSheet("background-color: lightgray; border-radius: 8px;");
    selectFrame->setFixedWidth(450);
}

void FileTransWidget::sendOptions()
{
    QStringList dirs;
    for (int i = 0; i < 5; i++) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(selectLayout->itemAtPosition(i, 0)->widget());
        if (checkBox && checkBox->isChecked()) {
            const QString dir = checkBox->text();
            dirs.append(dir);
        }
    }
    qInfo() << "select file :" << dirs;
    OptionsManager::instance()->addUserOption(Options::kFile, dirs);
}

void FileTransWidget::nextPage()
{
    //send useroptions
    sendOptions();

    //nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void FileTransWidget::update()
{
    int total = 0;
    QStringList dirs;
    for (int i = 0; i < 5; i++) {
        QCheckBox *checkBox = qobject_cast<QCheckBox *>(selectLayout->itemAtPosition(i, 0)->widget());
        if (checkBox && checkBox->isChecked()) {
            const QString dir = directories[i];
            total += userData[dir];
            dirs.append(dir);
        }
    }
    //update options
    OptionsManager::instance()->addUserOption(Options::kFile, dirs);

    //update ui
    QString StorageInfo = "数据盘可用空间%1G，已选数据共约%2G";
    StorageInfo = StorageInfo.arg(remainStorage).arg(QString::number(total));
    qInfo() << StorageInfo;
    storageInfoLabel->setText(StorageInfo);
}
