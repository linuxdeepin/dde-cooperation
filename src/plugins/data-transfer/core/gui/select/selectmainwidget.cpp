#include "selectmainwidget.h"

#include <QToolButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDebug>
#include <QLabel>
#include <gui/connect/choosewidget.h>
#include <gui/mainwindow_p.h>
#pragma execution_character_set("utf-8")

selectMainWidget::selectMainWidget(QWidget *parent) : QFrame(parent)
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    QLabel *textLabel = new QLabel("选择要传输的内容", this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel->setFont(font);
    textLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    SelectItem *fileItem =
            new SelectItem("文件", QIcon(":/icon/file.svg"), SelectItemName::FILES, this);
    SelectItem *appItem =
            new SelectItem("应用", QIcon(":/icon/app.svg"), SelectItemName::APP, this);
    SelectItem *dispositionItem = new SelectItem("配置", QIcon(":/icon/disposition.svg"),
                                                 SelectItemName::DISPOSITION, this);

    appItem->updateSelectSize(10);
    dispositionItem->updateSelectSize(9);
    QHBoxLayout *modeLayout = new QHBoxLayout(this);
    modeLayout->addWidget(fileItem, Qt::AlignTop);
    modeLayout->addSpacing(0);
    modeLayout->addWidget(appItem, Qt::AlignTop);
    modeLayout->addSpacing(0);
    modeLayout->addWidget(dispositionItem, Qt::AlignTop);
    modeLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    QObject::connect(backButton, &QToolButton::clicked, this, &selectMainWidget::backPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText("开始传输");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: #0098FF;");
    QObject::connect(nextButton, &QToolButton::clicked, this, &selectMainWidget::nextPage);

    QHBoxLayout *buttonLayout = new QHBoxLayout(this);
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    IndexLabel *indelabel = new IndexLabel(2, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout(this);
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addSpacing(60);
    mainLayout->addWidget(textLabel);
    mainLayout->addLayout(modeLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(indexLayout);
}

selectMainWidget::~selectMainWidget() { }
void selectMainWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void selectMainWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() - 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}

void selectMainWidget::selectPage()
{
    SelectItem *selectitem = qobject_cast<SelectItem*>(QObject::sender());
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    int pageNum = -1;
    if (selectitem->name == SelectItemName::FILES) {
        pageNum = data_transfer_core::PageName::filewselectidget;
    }else if(selectitem->name == SelectItemName::APP){
        pageNum = data_transfer_core::PageName::appselectwidget;
    }else if(selectitem->name == SelectItemName::DISPOSITION){
        pageNum = data_transfer_core::PageName::configselectwidget;
    }
    if(pageNum == -1)
    {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }else{
         stackedWidget->setCurrentIndex(pageNum);
    }
}

SelectItem::SelectItem(QString text, QIcon icon, SelectItemName itemName, QWidget *parent)
    : QFrame(parent), name(itemName)
{
    setStyleSheet(" background-color: rgb(0, 0, 0,0.03); border-radius: 8px;");
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

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(selectNname);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(sizeLabel);

    initEditFrame();

    QObject::connect(this,&SelectItem::changePage,qobject_cast<selectMainWidget*>(parent),&selectMainWidget::selectPage);
}
SelectItem::~SelectItem() { }

void SelectItem::updateSelectSize(int num)
{
    if (name == SelectItemName::APP) {
        sizeLabel->setText(QString("已选:%1").arg(num));
    } else if (name == SelectItemName::FILES) {
        sizeLabel->setText("");
    } else if (name == SelectItemName::DISPOSITION) {
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

void SelectItem::initEditFrame()
{
    editFrame = new QFrame(this);
    editFrame->setStyleSheet("background-color: rgba(0, 0, 0, 0.3);"); // 背景色设置为半透明黑色
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

void SelectItem::changeStyle()
{
    setStyleSheet("border-radius: 8px;"
                  "border: 1px solid rgba(0,129,255, 0.2);"
                  "opacity: 1;"
                  "background-color: rgba(0,129,255, 0.1);");
}
