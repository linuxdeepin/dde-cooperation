﻿#include "selectmainwidget.h"
#include "../type_defines.h"
#include <QToolButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <QLabel>
#include <QPainterPath>
#include <gui/connect/choosewidget.h>
#include <gui/mainwindow_p.h>
#include <utils/transferhepler.h>
#include <utils/optionsmanager.h>
#pragma execution_character_set("utf-8")

static inline constexpr char InternetText[]{ "请选择要传输的内容" };
static inline constexpr char LocalText[]{ "请选择要备份的内容" };
static inline constexpr char BtnInternetText[]{ "开始传输" };
static inline constexpr char BtnLocalText[]{ "下一步" };
SelectMainWidget::SelectMainWidget(QWidget *parent) : QFrame(parent)
{
    initUi();
}

SelectMainWidget::~SelectMainWidget() { }

void SelectMainWidget::changeSelectframeState(const SelectItemName &name, const bool &ok)
{
    if (name == SelectItemName::APP) {
        appItem->isOk = ok;
        appItem->update();
        QStringList list = OptionsManager::instance()->getUserOption(Options::kApp);
        appItem->updateSelectSize(list.size());
    } else if (name == SelectItemName::FILES) {
        fileItem->isOk = ok;
        fileItem->update();
        fileItem->updateSelectSize(9);
    } else if (name == SelectItemName::CONFIG) {
        configItem->isOk = ok;
        configItem->update();
        QStringList ConfigList = OptionsManager::instance()->getUserOption(Options::kConfig);
        QStringList BrowserList =
                OptionsManager::instance()->getUserOption(Options::kBrowserBookmarks);
        configItem->updateSelectSize(ConfigList.size() + BrowserList.size());
    }
}

void SelectMainWidget::changeText()
{
    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        titileLabel->setText(LocalText);
        nextButton->setText(BtnLocalText);
    } else if (method == TransferMethod::kNetworkTransmission) {
        titileLabel->setText(InternetText);
        nextButton->setText(BtnInternetText);
    }
}

void SelectMainWidget::initUi()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    titileLabel = new QLabel(LocalText, this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    fileItem = new SelectItem("文件", QIcon(":/icon/file.svg"), SelectItemName::FILES, this);
    appItem = new SelectItem("应用", QIcon(":/icon/app.svg"), SelectItemName::APP, this);
    configItem =
            new SelectItem("配置", QIcon(":/icon/disposition.svg"), SelectItemName::CONFIG, this);

    QHBoxLayout *modeLayout = new QHBoxLayout();

    modeLayout->addWidget(fileItem, Qt::AlignTop);
    modeLayout->addSpacing(0);
    modeLayout->addWidget(appItem, Qt::AlignTop);
    modeLayout->addSpacing(0);
    modeLayout->addWidget(configItem, Qt::AlignTop);
    modeLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet(".QToolButton{border-radius: 8px;"
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
    QObject::connect(backButton, &QToolButton::clicked, this, &SelectMainWidget::backPage);

    nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText(BtnLocalText);
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet(".QToolButton{"
                              "border-radius: 8px;"
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

    QObject::connect(nextButton, &QToolButton::clicked, this, &SelectMainWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(2, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addSpacing(60);
    mainLayout->addWidget(titileLabel);
    mainLayout->addLayout(modeLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(indexLayout);
}
void SelectMainWidget::nextPage()
{
    PageName next;
    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        next = PageName::createbackupfilewidget;
    } else if (method == TransferMethod::kNetworkTransmission) {
        next = PageName::transferringwidget;
        // transfer
        TransferHelper::instance()->startTransfer();
    }

    // ui
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(next);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void SelectMainWidget::backPage()
{
    PageName back;
  //  QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
  //  if (method == TransferMethod::kLocalExport) {
      //  back = PageName::choosewidget;
//    } else if (method == TransferMethod::kNetworkTransmission) {
        back = PageName::readywidget;
//    }
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(back);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void SelectMainWidget::selectPage()
{
    SelectItem *selectitem = qobject_cast<SelectItem *>(QObject::sender());
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    int pageNum = -1;
    if (selectitem->name == SelectItemName::FILES) {
        pageNum = PageName::filewselectidget;
    } else if (selectitem->name == SelectItemName::APP) {
        pageNum = PageName::appselectwidget;
    } else if (selectitem->name == SelectItemName::CONFIG) {
        pageNum = PageName::configselectwidget;
    }
    if (pageNum == -1) {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    } else {
        stackedWidget->setCurrentIndex(pageNum);
    }
}

SelectItem::SelectItem(QString text, QIcon icon, SelectItemName itemName, QWidget *parent)
    : QFrame(parent), name(itemName)
{
    setStyleSheet(" background-color: rgba(0, 0, 0,0.08); border-radius: 8px;");
    setFixedSize(160, 185);

    QLabel *selectNname = new QLabel(text, this);
    selectNname->setAlignment(Qt::AlignCenter);
    selectNname->setStyleSheet("background-color: rgba(0, 0, 0, 0);");

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(icon.pixmap(100, 80));
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");

    sizeLabel = new QLabel(this);
    sizeLabel->setText("已选:0");
    sizeLabel->setAlignment(Qt::AlignCenter);
    sizeLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(selectNname);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(sizeLabel);

    initEditFrame();

    QObject::connect(this, &SelectItem::changePage, qobject_cast<SelectMainWidget *>(parent),
                     &SelectMainWidget::selectPage);
}
SelectItem::~SelectItem() { }

void SelectItem::updateSelectSize(int num)
{
    if (name == SelectItemName::APP) {
        sizeLabel->setText(QString("已选:%1").arg(num));
    } else if (name == SelectItemName::FILES) {
        sizeLabel->setText(QString("已选:%1GB").arg(num));
    } else if (name == SelectItemName::CONFIG) {
        sizeLabel->setText(QString("已选:%1项").arg(num));
    } else {
        qDebug() << "selectItemName is error!";
    }
}
void SelectItem::mousePressEvent(QMouseEvent *event)
{
    emit changePage();
}

void SelectItem::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
    editFrame->show();
}

void SelectItem::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    editFrame->hide();
}

void SelectItem::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    if (isOk) {
        this->setStyleSheet(".SelectItem{background-color:rgba(0,129,255,0.1);}");
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.moveTo(QPoint(this->width() - 16, 0));
        path.arcTo(this->width() - 16, 0, 16, 16, 0, 90);
        path.lineTo(QPoint(this->width() - 30, 0));
        path.lineTo(QPoint(this->width(), 34));
        path.lineTo(QPoint(this->width(), 8));
        painter.fillPath(path, QColor(0, 129, 255));

        QPixmap iconPixmap(":/icon/checked.svg");
        QSize scaledSize(13, 11);
        QPixmap scaledPixmap = iconPixmap.scaled(scaledSize);
        QRect iconRect(this->width() - 15, 5, scaledSize.width(), scaledSize.height());
        painter.drawPixmap(iconRect, scaledPixmap);
    } else {
        this->setStyleSheet(".SelectItem{background-color: rgba(0, 0, 0,0.08);}");
    }
}
void SelectItem::initEditFrame()
{
    editFrame = new QFrame(this);
    editFrame->setStyleSheet("background-color: rgba(0, 0, 0, 0.3);");
    editFrame->setGeometry(0, 0, width(), height());
    editFrame->hide();

    QLabel *iconLabel = new QLabel(editFrame);
    iconLabel->setPixmap(QIcon(":/icon/edit.svg").pixmap(24, 24));
    iconLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");

    QLabel *textLabel = new QLabel(editFrame);
    textLabel->setText("编辑");
    textLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);"
                             "font-family: \"SourceHanSansSC-Medium\";"
                             "color:white;"
                             "font-size: 14px;"
                             "font-weight: 500;"
                             "font-style: normal;"
                             "letter-spacing: 5px;"
                             "text-align: center;");
    textLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QVBoxLayout *editLayout = new QVBoxLayout(editFrame);

    editFrame->setLayout(editLayout);
    editLayout->addWidget(iconLabel);
    editLayout->addWidget(textLabel);
    editLayout->setSpacing(10);
}

void SelectItem::changeState(const bool &ok)
{
    isOk = ok;
    update();
}
