#include "uploadfilewidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QFileDialog>

#include <gui/connect/choosewidget.h>

#pragma execution_character_set("utf-8")

UploadFileWidget::UploadFileWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

UploadFileWidget::~UploadFileWidget()
{
}

void UploadFileWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel("选择信息迁移文件", this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    UploadFileFrame *uploadFileFrame = new UploadFileFrame(this);
    QHBoxLayout *uploadLayout = new QHBoxLayout(this);
    uploadLayout->addWidget(uploadFileFrame, Qt::AlignCenter);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &UploadFileWidget::backPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText("下一步");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: #0098FF;");
    connect(nextButton, &QToolButton::clicked, this, &UploadFileWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    IndexLabel *indelabel = new IndexLabel(1, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(uploadLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);
}

void UploadFileWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void UploadFileWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

UploadFileFrame::UploadFileFrame(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

UploadFileFrame::~UploadFileFrame()
{
}

void UploadFileFrame::initUI()
{
    setStyleSheet("background-color: rgba(0, 0, 0, 0.03); border-radius: 10px;");
    setFixedSize(460, 275);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/zip-64.svg").pixmap(64, 64));
    iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *textLabel = new QLabel("拖拽文件至或", this);
    textLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
    textLabel->setFixedHeight(20);
    textLabel->setAlignment(Qt::AlignCenter);

    QString display = "<a href=\"https://\" style=\"text-decoration:none;\">上传文件</a>";
    QLabel *displayLabel = new QLabel(display, this);
    displayLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
    displayLabel->setAlignment(Qt::AlignCenter);
    displayLabel->setFixedHeight(20);
    connect(displayLabel, &QLabel::linkActivated, this, &UploadFileFrame::uploadFile);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel);
    mainLayout->addWidget(displayLabel);
    mainLayout->addSpacing(70);
}

void UploadFileFrame::uploadFile()
{
    selectedFilePath = QFileDialog::getOpenFileName(nullptr, "选择zip文件", "", "ZIP 文件 (*.zip)");

    if (!selectedFilePath.isEmpty())
        update();
}

void UploadFileFrame::update()
{
    if (!selectedFilePath.isEmpty()) {

    } else {
        // 用户取消了选择
        qDebug() << "用户取消了选择";
    }
}
