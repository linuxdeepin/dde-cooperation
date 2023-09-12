#include "filetranswidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>

FileTransWidget::FileTransWidget(QWidget *parent)
    : QFrame(parent)
{
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
    QLabel *tipLabel1 = new QLabel("windows user", this);
    QHBoxLayout *layout1 = new QHBoxLayout(this);
    layout1->addSpacing(50);
    layout1->addWidget(tipLabel1, Qt::AlignCenter);

    QGridLayout *layout = new QGridLayout(this);
    for (int i = 0; i < 5; i++) {
        QCheckBox *checkBox1 = new QCheckBox("文档", selectFrame);
        QLabel *label1 = new QLabel("13G", selectFrame);
        layout->addWidget(checkBox1, i, 0);
        layout->addWidget(label1, i, 1);
    }
    QHBoxLayout *layout2 = new QHBoxLayout(this);
    layout2->addSpacing(50);
    layout2->addLayout(layout);

    QLabel *tipLabel2 = new QLabel("数据盘可用空间128G，已选数据共约11G", this);
    tipLabel2->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
    QFont font;
    font.setPointSize(8);
    tipLabel2->setFont(font);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(layout1);
    mainLayout->addLayout(layout2);
    mainLayout->addWidget(tipLabel2);

    selectFrame->setLayout(mainLayout);
    selectFrame->setStyleSheet("background-color: lightgray; border-radius: 8px;");
    selectFrame->setFixedWidth(450);
}

void FileTransWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(stackedWidget->currentIndex() + 1);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}
