#include "../mainwindow.h"
#include "../mainwindow_p.h"

#include <QLabel>

using namespace data_transfer_core;

void MainWindowPrivate::initWindow()
{
    q->setWindowTitle("Hello world");
}


void MainWindowPrivate::initWidgets()
{
    QWidget *centerW = new QWidget(q);
    QLabel *lab = new QLabel(centerW);
    lab->setText("数据传输工具");
    lab->setAlignment(Qt::AlignCenter);
    QVBoxLayout *lay = new QVBoxLayout(centerW);
    lay->addWidget(lab);
    q->setCentralWidget(centerW);
}

