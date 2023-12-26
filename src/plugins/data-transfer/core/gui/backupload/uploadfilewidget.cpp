#include "uploadfilewidget.h"
#include "unzipwoker.h"

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

#include <utils/transferhepler.h>

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
    setStyleSheet(".UploadFileWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(30);

    QLabel *titileLabel = new QLabel(tr("Select data transfer file"), this);
    titileLabel->setFixedHeight(50);
    QFont font;
    font.setPixelSize(24);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    uploadFileFrame = new UploadFileFrame(this);
    QHBoxLayout *uploadLayout = new QHBoxLayout();
    uploadLayout->addWidget(uploadFileFrame, Qt::AlignCenter);

    tipLabel = new QLabel(QString("<font size=12px color='#FF5736' >%1</font>").arg(tr("File error, cannot transfer, please reselect")), this);
    tipLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    tipLabel->setFixedHeight(20);
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setVisible(false);

    QHBoxLayout *tipLayout = new QHBoxLayout();
    tipLayout->addSpacing(15);
    tipLayout->addWidget(tipLabel);
    tipLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    backButton = new QToolButton(this);
    backButton->setText(tr("Back"));
    backButton->setFixedSize(120, 35);
    connect(backButton, &QToolButton::clicked, this, &UploadFileWidget::backPage);

    nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText(tr("Next"));
    nextButton->setFixedSize(120, 35);

    nextButton->setStyleSheet("QToolButton[Theme=\"true\"][Enabled=\"true\"] { background-color: rgba(0,152, 255,1);border-radius: 8px; }"
                              "QToolButton[Theme=\"true\"][Enabled=\"false\"] {background-color: rgba(0,152, 255,0.5);border-radius: 8px; }"
                              "QToolButton[Theme=\"false\"][Enabled=\"true\"] { background-color: rgba(0,152, 255,0.6);color: lightgray;border-radius: 8px; }"
                              "QToolButton[Theme=\"false\"][Enabled=\"false\"] { background-color: rgba(0,152, 255,0.3);color: lightgray; border-radius: 8px; }");

    nextButton->setEnabled(false);
    updataNextBut();
    connect(nextButton, &QToolButton::clicked, this, [this]() {
        if (nextButton->text() == tr("Retry")) {
            emit uploadFileFrame->updateUI(uploadStatus::Initial);
            tipLabel->setVisible(false);
            return;
        }
        if (!checkBackupFile(uploadFileFrame->getZipFilePath())) {
            tipLabel->setVisible(true);
            nextButton->setText(tr("Retry"));
            return;
        }
        UnzipWorker *woker = new UnzipWorker(uploadFileFrame->getZipFilePath());
        woker->start();
        nextPage();
    });

    connect(this, &UploadFileWidget::Initial, this, [this]() {
        emit uploadFileFrame->updateUI(uploadStatus::Initial);
        return;
    });

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(10);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    IndexLabel *indelabel = new IndexLabel(1, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(uploadLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(tipLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);

    connect(uploadFileFrame, &UploadFileFrame::updateUI, this, [this, indelabel](int status) {
        tipLabel->setVisible(false);
        if (status == uploadStatus::valid) {
            nextButton->setEnabled(true);
            indelabel->setIndex(2);
        } else {
            indelabel->setIndex(1);
            nextButton->setEnabled(false);
            nextButton->setText(tr("Next"));
        }
        updataNextBut();
    });
}

void UploadFileWidget::updataNextBut()
{
    nextButton->setProperty("Theme", currentState);
    nextButton->setProperty("Enabled", nextButton->isEnabled());
    nextButton->style()->unpolish(nextButton);
    nextButton->style()->polish(nextButton);
}

bool UploadFileWidget::checkBackupFile(const QString &filePath)
{
    //Verify effectiveness
    if (!UnzipWorker::isValid(filePath)) {
        tipLabel->setText(QString("<font size=12px color='#FF5736' >%1</font>").arg(tr("The file is corrupted and cannot be migrated. Please replace it with a backup file.")));
        tipLabel->setVisible(true);
        return false;
    }

    // Verify size
    QFileInfo info(filePath);
    LOG << "checkBackupFile .size();" << info.size();
    int size = static_cast<int>(info.size() / 1024 / 1024 / 1024) * 2;
    if (size > TransferHelper::instance()->getRemainSize()) {
        tipLabel->setVisible(true);
        tipLabel->setText(QString("<font size=12px color='#FF5736' >%1</font>").arg(tr("Insufficient space on UOS. Please reserve at least %1G of space and try again.").arg(size)));
        return false;
    }
    return true;
}

void UploadFileWidget::clear()
{
    emit Initial();
}

void UploadFileWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::transferringwidget);
    } else {
        WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void UploadFileWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void UploadFileWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        setStyleSheet(".UploadFileWidget{background-color: white; border-radius: 10px;}");

        backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "background-color: lightgray;"
                                  "}");
        uploadFileFrame->setStyleSheet(".UploadFileFrame{background-color: rgba(0, 0, 0, 0.03);"
                                       "border-radius: 10px;"
                                       "border-style: dashed;"
                                       "border-width: 2px;"
                                       "border-color: rgba(0, 0, 0, 0.06);}");
       currentState = true;
    } else {
        //dark
        setStyleSheet(".UploadFileWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "opacity: 1;"
                                  "background-color: rgba(255,255,255, 0.1);"
                                  "}");
        uploadFileFrame->setStyleSheet(".UploadFileFrame{"
                                       "background-color: rgba(255,255,255, 0.05);"
                                       "border-radius: 10px;"
                                       "border-style: dashed;"
                                       "border-width: 2px;"
                                       "border-color: rgba(255,255,255, 0.1);}");
         currentState = false;
    }
    updataNextBut();
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
    setFixedSize(460, 275);
    setAcceptDrops(true);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/zip-64.svg").pixmap(64, 64));
    iconLabel->setStyleSheet(".QLabel{background-color: rgba(0, 0, 0, 0);border-style: none;}");
    iconLabel->setAlignment(Qt::AlignCenter);

    QLabel *textLabel = new QLabel(QString("<font color='gray' >%1</font>").arg(tr("Drag file here ")), this);
    textLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    textLabel->setFixedHeight(20);
    textLabel->setAlignment(Qt::AlignCenter);

    QString display = QString("<a href=\"https://\" style=\"text-decoration:none;\">%1</a>").arg(tr("Import file"));
    QLabel *displayLabel = new QLabel(display, this);
    displayLabel->setStyleSheet(".QLabel{background-color: rgba(0, 0, 0, 0);border-style: none;}");
    displayLabel->setAlignment(Qt::AlignCenter);
    displayLabel->setFixedHeight(20);
    QFont tipfont;
    tipfont.setPixelSize(12);
    displayLabel->setFont(tipfont);
    textLabel->setFont(tipfont);
    connect(displayLabel, &QLabel::linkActivated, this, &UploadFileFrame::uploadFile);

    QPushButton *closeBtn = new QPushButton(this);
    closeBtn->setIcon(QIcon(":/icon/tab_close_normal.svg"));
    closeBtn->setWindowOpacity(1.0);
    closeBtn->setStyleSheet("background-color: rgba(0, 0, 0, 0);border-style: none;");
    closeBtn->setIconSize(QSize(35, 35));
    closeBtn->setGeometry(270, 55, 35, 35);
    closeBtn->setVisible(false);

    QLabel *WarningIconLabel = new QLabel(iconLabel);
    WarningIconLabel->setPixmap(QIcon(":/icon/warning.svg").pixmap(30, 30));
    WarningIconLabel->setStyleSheet(".QLabel{background-color: rgba(0, 0, 0, 0);border-style: none;}");
    WarningIconLabel->setGeometry(230, 60, 30, 30);
    WarningIconLabel->setVisible(false);

    initFileFrame();
    QHBoxLayout *fileHLayout = new QHBoxLayout(fileFrame);
    fileHLayout->addWidget(fileFrame);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addSpacing(50);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(textLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(displayLabel);
    mainLayout->addLayout(fileHLayout);
    mainLayout->addSpacing(70);

    connect(closeBtn, &QPushButton::clicked, this, [this] {
        emit updateUI(uploadStatus::Initial);
    });

    connect(this, &UploadFileFrame::updateUI, this, [WarningIconLabel, this, closeBtn, iconLabel, textLabel, displayLabel](int status) {
        switch (status) {
        case uploadStatus::Initial: {
            fileFrame->setVisible(false);
            closeBtn->setVisible(false);
            WarningIconLabel->setVisible(false);
            iconLabel->setVisible(true);
            textLabel->setVisible(true);
            displayLabel->setVisible(true);
            textLabel->setText(QString("<font color='gray' >%1</font>").arg(tr("Drag file here")));
            displayLabel->setText(QString("<a href=\"https://\" style=\"text-decoration:none;font-size:12px;\">%1</a>").arg(tr("Import file")));
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
            textLabel->setText(QString("<font color='gray' >%1</font>").arg(tr("Only .zip is supported, please")));
            displayLabel->setText(QString("<a href=\"https://\" style=\"text-decoration:none;font-size:12px;\">%1</a>").arg(tr("reselect")));
            break;
        }
        default:
            break;
        }
    });
}

void UploadFileFrame::initStyleSheet()
{
    setStyleSheet(".UploadFileFrame{background-color: rgba(0, 0, 0, 0.03);"
                  "border-radius: 10px;"
                  "border-style: dashed;"
                  "border-width: 2px;"
                  "border-color: rgba(0, 0, 0, 0.06);}");
}

void UploadFileFrame::initFileFrame()
{
    fileFrame = new QFrame(this);
    fileFrame->setStyleSheet(".QFrame{background-color: rgba(0, 0, 0, 0.1);border-style: none; border-radius: 10px;}");
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
        if (status == uploadStatus::valid) {
            QFileInfo info(zipFilePath);
            textLabel->setText("<font size=12px color='gray' >" + info.fileName() + " </font>");
        }
    });
}

QString UploadFileFrame::getZipFilePath() const
{
    return zipFilePath;
}

void UploadFileFrame::uploadFile()
{
    zipFilePath = QFileDialog::getOpenFileName(nullptr, tr("select zip file"), "", tr("ZIP file (*.zip)"));
    LOG << "set zipFilePath =" + zipFilePath.toStdString();
    if (!zipFilePath.isEmpty())
        emit updateUI(uploadStatus::valid);
}

void UploadFileFrame::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void UploadFileFrame::dragMoveEvent(QDragMoveEvent *event)
{
    setStyleSheet(".UploadFileFrame{background-color: rgba(0, 129, 255, 0.2);"
                  "border-radius: 10px;"
                  "border-style: dashed;"
                  "border-width: 2px;"
                  "border-color: rgba(0, 0, 0, 0.06);}");
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
    QUrl url = urls.first();
    QFileInfo info(url.url());
    if (info.suffix() != "zip") {
        emit updateUI(uploadStatus::formaterror);
        return;
    } else {
        zipFilePath = url.path();
        LOG << "set zipFilePath =" + zipFilePath.toStdString();
        if (!zipFilePath.isEmpty())
            emit updateUI(uploadStatus::valid);
    }
}
