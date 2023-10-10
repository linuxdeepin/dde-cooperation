#include "uploadfilewidget.h"

#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QFileDialog>
#include <QPushButton>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

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
        stackedWidget->setCurrentIndex(PageName::transferringwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void UploadFileWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
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
    initStyleSheet();
    setFixedSize(460, 275);
    setAcceptDrops(true);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/zip-64.svg").pixmap(64, 64));
    iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *textLabel = new QLabel("<font size=12px color='gray' >拖拽文件至或</font>", this);
    textLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    textLabel->setFixedHeight(20);
    textLabel->setAlignment(Qt::AlignCenter);

    QString display = "<a href=\"https://\" style=\"text-decoration:none;font-size:12px;\">上传文件</a>";
    QLabel *displayLabel = new QLabel(display, this);
    displayLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    displayLabel->setAlignment(Qt::AlignCenter);
    displayLabel->setFixedHeight(20);
    connect(displayLabel, &QLabel::linkActivated, this, &UploadFileFrame::uploadFile);

    QToolButton *closeBtn = new QToolButton(this);
    closeBtn->setIcon(QIcon(":/icon/tab_close_normal.svg"));
    closeBtn->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    closeBtn->setIconSize(QSize(35, 35));
    closeBtn->setGeometry(270, 55, 35, 35);
    closeBtn->setVisible(false);

    QLabel *WarningIconLabel = new QLabel(iconLabel);
    WarningIconLabel->setPixmap(QIcon(":/icon/warning.svg").pixmap(30, 30));
    WarningIconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    WarningIconLabel->setGeometry(230, 60, 30, 30);
    WarningIconLabel->setVisible(false);

    initFileFrame();
    QHBoxLayout *fileHLayout = new QHBoxLayout(fileFrame);
    fileHLayout->addWidget(fileFrame);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(displayLabel);
    mainLayout->addLayout(fileHLayout);
    mainLayout->addSpacing(70);

    connect(closeBtn, &QToolButton::clicked, this, [this] {
        emit updateUI(uploadStatus::invalid);
    });

    connect(this, &UploadFileFrame::updateUI, this, [WarningIconLabel, this, closeBtn, iconLabel, textLabel, displayLabel](int status) {
        switch (status) {
        case uploadStatus::invalid: {
            fileFrame->setVisible(false);
            closeBtn->setVisible(false);
            WarningIconLabel->setVisible(false);
            iconLabel->setVisible(true);
            textLabel->setVisible(true);
            displayLabel->setVisible(true);
            textLabel->setText("<font size=12px color='gray' >拖拽文件至或</font>");
            displayLabel->setText("<a href=\"https://\" style=\"text-decoration:none;font-size:12px;\">上传文件</a>");
            break;
        }
        case uploadStatus::valid: {
            fileFrame->setVisible(true);
            closeBtn->setVisible(true);
            WarningIconLabel->setVisible(false);
            iconLabel->setVisible(false);
            textLabel->setVisible(false);
            displayLabel->setVisible(false);
            break;
        }
        case uploadStatus::formaterror: {
            fileFrame->setVisible(false);
            closeBtn->setVisible(false);
            iconLabel->setVisible(true);
            WarningIconLabel->setVisible(true);
            textLabel->setText("<font size=12px color='gray' >抱歉，只支持zip格式，请</font>");
            displayLabel->setText("<a href=\"https://\" style=\"text-decoration:none;font-size:12px;\">重新上传</a>");
            break;
        }
        default:
            break;
        }
    });
}

void UploadFileFrame::initStyleSheet()
{
    setStyleSheet("background-color: rgba(0, 0, 0, 0.03);"
                  "border-radius: 10px;"
                  "border-style: dashed;"
                  "border-width: 2px;"
                  "border-color: rgba(0, 0, 0, 0.06);");
}

void UploadFileFrame::initFileFrame()
{
    fileFrame = new QFrame(this);
    fileFrame->setStyleSheet("background-color: rgba(0, 0, 0, 0.06);border-style: none;");
    fileFrame->setFixedSize(124, 111);
    fileFrame->setVisible(false);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/application-x-zip.svg").pixmap(64, 64));
    iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *textLabel = new QLabel(this);
    textLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    textLabel->setFixedHeight(20);
    textLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *fileFrameLayout = new QVBoxLayout();
    fileFrameLayout->addWidget(iconLabel);
    fileFrameLayout->addWidget(textLabel);
    fileFrame->setLayout(fileFrameLayout);

    connect(this, &UploadFileFrame::updateUI, this, [this, textLabel](int status) {
        if (status == uploadStatus::valid)
            textLabel->setText("<font size=12px color='gray' >" + selectedFilePath + " </font>");
    });
}

void UploadFileFrame::uploadFile()
{
    selectedFilePath = QFileDialog::getOpenFileName(nullptr, "选择zip文件", "", "ZIP 文件 (*.zip)");
    QFileInfo info(selectedFilePath);
    selectedFilePath = info.fileName();
    if (!selectedFilePath.isEmpty())
        emit updateUI(uploadStatus::valid);
}

void UploadFileFrame::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void UploadFileFrame::dragMoveEvent(QDragMoveEvent *event)
{
    setStyleSheet("background-color: rgba(0, 129, 255, 0.2);");
    event->accept();
}

void UploadFileFrame::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    initStyleSheet();
}

void UploadFileFrame::dropEvent(QDropEvent *event)
{
    initStyleSheet();
    const QList<QUrl> &urls = event->mimeData()->urls();
    if (urls.size() != 1)
        return;
    QFileInfo info(urls.first().url());
    if (info.suffix() != "zip") {
        emit updateUI(uploadStatus::formaterror);
        return;
    } else {
        selectedFilePath = info.fileName();
        if (!selectedFilePath.isEmpty())
            emit updateUI(uploadStatus::valid);
    }
}
