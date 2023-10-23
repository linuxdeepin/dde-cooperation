#include "resultdisplay.h"
#include "../type_defines.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QApplication>
#include <QStandardItemModel>

#include <gui/mainwindow_p.h>

#include <utils/transferhepler.h>
#pragma execution_character_set("utf-8")

ResultDisplayWidget::ResultDisplayWidget(QWidget *parent)
    : QFrame(parent)
{
    initUI();
}

ResultDisplayWidget::~ResultDisplayWidget()
{
}

void ResultDisplayWidget::initUI()
{
    initListTitle();
    initListView();

    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success half-96.svg").pixmap(73, 73));
    iconLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    QLabel *titileLabel = new QLabel("部分迁移完成", this);
    titileLabel->setStyleSheet("color: black;"
                               "font-size: 24px;"
                               "font-weight: 700;");
    titileLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    QHBoxLayout *titilelayout = new QHBoxLayout(this);
    titilelayout->addWidget(iconLabel);
    titilelayout->addSpacing(5);
    titilelayout->addWidget(titileLabel);
    titilelayout->addSpacing(50);

    QLabel *tipiconlabel = new QLabel(this);
    tipiconlabel->setPixmap(QIcon(":/icon/dialog-warning.svg").pixmap(14, 14));

    QLabel *tiptextlabel = new QLabel(this);
    tiptextlabel->setText("<font size=12px color='gray' >部分信息迁移失败，请手动迁移。</font>");

    QHBoxLayout *tiplayout = new QHBoxLayout(this);
    tiplayout->addSpacing(95);
    tiplayout->addWidget(tipiconlabel);
    tiplayout->addWidget(tiptextlabel);
    tiplayout->setAlignment(Qt::AlignLeft);

    QHBoxLayout *listlayout = new QHBoxLayout();
    listlayout->addWidget(listview);

    QToolButton *backButton = new QToolButton(this);
    backButton->setText("返回");
    backButton->setFixedSize(120, 35);
    backButton->setStyleSheet("background-color: lightgray;");
    connect(backButton, &QToolButton::clicked, this, &ResultDisplayWidget::nextPage);

    QToolButton *nextButton = new QToolButton(this);
    QPalette palette = nextButton->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    nextButton->setPalette(palette);
    nextButton->setText("退出");
    nextButton->setFixedSize(120, 35);
    nextButton->setStyleSheet("background-color: #0098FF;");
    connect(nextButton, &QToolButton::clicked, qApp, &QApplication::quit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(nextButton);
    buttonLayout->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);

    mainLayout->addSpacing(30);
    mainLayout->addLayout(titilelayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(tiplayout);
    mainLayout->addWidget(listTitle);
    mainLayout->addLayout(listlayout);
    mainLayout->addSpacing(300);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(5);

    connect(TransferHelper::instance(), &TransferHelper::failure, this, &ResultDisplayWidget::addFailure);
}

void ResultDisplayWidget::initListTitle()
{
    QLabel *text1 = new QLabel("内容", this);
    QLabel *text2 = new QLabel("类别", this);
    QLabel *text3 = new QLabel("失败原因", this);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addSpacing(90);
    layout->addWidget(text1);
    layout->addSpacing(200);
    layout->addWidget(text2);
    layout->addSpacing(100);
    layout->addWidget(text3);
    layout->setAlignment(Qt::AlignLeft);

    listTitle = new QFrame(this);
    listTitle->setLayout(layout);
}

void ResultDisplayWidget::initListView()
{
    listview = new QListView(this);
    listview->setSelectionMode(QAbstractItemView::NoSelection);
    listview->setFixedSize(520, 235);
    listview->setStyleSheet("background-color:rgba(0, 0, 0, 0.03);");
    QStandardItemModel *model = new QStandardItemModel();
    listview->setModel(model);
    listview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listview->setItemDelegate(new itemDelegate());
}

void ResultDisplayWidget::nextPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::choosewidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = nullptr";
    }
}

void ResultDisplayWidget::themeChanged(int theme)
{
    //light
    if (theme == 1) {
        listTitle->setStyleSheet("background-color: white; border-radius: 10px;");
        setStyleSheet("background-color: white; border-radius: 10px;");
    } else {
        //dark
        listTitle->setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
    }
}

void ResultDisplayWidget::addFailure(QString name, QString type, QString reason)
{
    auto model = qobject_cast<QStandardItemModel *>(listview->model());
    QStandardItem *item = new QStandardItem();
    item->setData(name, Qt::DisplayRole);
    item->setData(type, Qt::ToolTipRole);
    item->setData(reason, Qt::UserRole);
    model->appendRow(item);
}

itemDelegate::itemDelegate()
{
}

itemDelegate::~itemDelegate()
{
}

void itemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    paintBackground(painter, option, index);
    paintText(painter, option, index);
}

QSize itemDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(180, 36);
}

void itemDelegate::paintText(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    painter->setPen(QColor("#526A7F"));
    QFont font;
    font.setPixelSize(12);
    painter->setFont(font);

    QRect namePos = option.rect.adjusted(20, 0, 0, 0);
    QString fileText = index.data(Qt::DisplayRole).toString();
    painter->drawText(namePos, Qt::AlignLeft | Qt::AlignVCenter, fileText);

    QRect tpyePos = option.rect.adjusted(220, 0, 0, 0);
    QString tpyeText = index.data(Qt::ToolTipRole).toString();
    painter->drawText(tpyePos, Qt::AlignLeft | Qt::AlignVCenter, tpyeText);

    painter->setPen(QColor("#FF5736"));
    QRect reasonPos = option.rect.adjusted(370, 0, 0, 0);
    QString reasonText = index.data(Qt::UserRole).toString();
    painter->drawText(reasonPos, Qt::AlignLeft | Qt::AlignVCenter, reasonText);

    painter->restore();
}

void itemDelegate::paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    QRect positon(option.rect);
    painter->setPen(Qt::NoPen);
    if (index.row() % 2 == 0) {
        painter->setBrush(QColor(0, 0, 0, 12));
        painter->drawRoundedRect(positon, 8, 8);
    }
    painter->restore();
}
