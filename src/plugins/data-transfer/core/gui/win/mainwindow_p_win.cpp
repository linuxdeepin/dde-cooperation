#include "../mainwindow.h"
#include "../mainwindow_p.h"
#include "../startwidget.h"
#include "../searchwidget.h"
#include "../connectwidget.h"
#include "../filetranswidget.h"
#include "../apptranswidget.h"
#include "../configtranswidget.h"
#include "../transferringwidget.h"
#include "../successwidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolBar>
#include <QDesktopWidget>
#include <QScreen>
#include <QToolButton>
using namespace data_transfer_core;

void MainWindowPrivate::initWindow()
{
    q->setWindowFlags(Qt::FramelessWindowHint);
    q->setAttribute(Qt::WA_TranslucentBackground);
    q->setFixedSize(800, 500);
    QWidget *centerWidget = new QWidget(q);
    QVBoxLayout *layout = new QVBoxLayout(centerWidget);
    centerWidget->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);  // 设置边距为0

    q->setCentralWidget(centerWidget);

    windowsCentralWidget = new QVBoxLayout(centerWidget);
    windowsCentralWidget->setContentsMargins(8,5,8,8);

    QWidget *menuBar = new QWidget(centerWidget);
    {
        menuBar->setStyleSheet("QWidget {"
                               "background-color: white;" // 设置背景颜色
                               "border-top-left-radius: 20px;" // 左上角圆角半径
                               "border-top-right-radius: 20px;" // 右上角圆角半径
                               "}");
        QToolButton *close = new QToolButton(menuBar);
        close->setStyleSheet("background-color: red;");
        close->setFixedSize(30,30);
        QToolButton *min = new QToolButton(menuBar);
        min->setStyleSheet("background-color: yellow;");
        min->setFixedSize(30,30);
        QToolButton *max = new QToolButton(menuBar);
        max->setStyleSheet("background-color: green;");
        max->setFixedSize(30,30);
        QToolButton *help = new QToolButton(menuBar);
        help->setStyleSheet("background-color: black;");
        help->setFixedSize(30,30);

        QLabel *mainLabel = new QLabel(menuBar);
        QPixmap iconPixmap("C:\\Users\\deep\\Documents\\test01\\uos.png"); // 替换为您的图标文件的路径
        mainLabel->setPixmap(iconPixmap.scaled(100, 50, Qt::KeepAspectRatio));

        QObject::connect(close, &QToolButton::clicked, q, &QWidget::close);
        QObject::connect(max, &QToolButton::clicked, q, &MainWindow::showMaximized);
        QObject::connect(min, &QToolButton::clicked, q, &MainWindow::showMinimized);
        QSpacerItem* spacer=new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        menuBar->setFixedWidth(q->width());
        menuBar->setFixedHeight(50);
        QHBoxLayout *menuHlayout = new QHBoxLayout(centerWidget);
        menuHlayout->addWidget(mainLabel);
        menuHlayout->addSpacerItem(spacer);
        menuHlayout->addWidget(help);
        menuHlayout->addWidget(min);
        menuHlayout->addWidget(max);
        menuHlayout->addWidget(close);
        menuBar->setLayout(menuHlayout);
        menuHlayout->setContentsMargins(8,8,8,8);
    }


    layout->addWidget(menuBar,Qt::AlignTop );
    layout->addLayout(windowsCentralWidget);
}


void MainWindowPrivate::initWidgets()
{
    StartWidget *startwidget = new StartWidget(q);
    SearchWidget *searchwidget = new SearchWidget(q);
    ConnectWidget *connectwidget = new ConnectWidget(q);
    FileTransWidget *filetranswidget = new FileTransWidget(q);
    AppTransWidget *apptranswidget = new AppTransWidget(q);
    ConfigTransWidget *configtranswidget = new ConfigTransWidget(q);
    TransferringWidget *transferringwidget = new TransferringWidget(q);
    SuccessWidget *successtranswidget = new SuccessWidget(q);

    QStackedWidget *stackedWidget = new QStackedWidget(q);
    stackedWidget->addWidget(startwidget);
    stackedWidget->addWidget(searchwidget);
    stackedWidget->addWidget(connectwidget);
    stackedWidget->addWidget(filetranswidget);
    stackedWidget->addWidget(apptranswidget);
    stackedWidget->addWidget(configtranswidget);
    stackedWidget->addWidget(transferringwidget);
    stackedWidget->addWidget(successtranswidget);
    stackedWidget->setCurrentIndex(0);

    windowsCentralWidget->addWidget(stackedWidget);
}

