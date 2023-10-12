#include "configselectwidget.h"
#include "appselectwidget.h"
#include "item.h"
#include "../type_defines.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QScrollArea>
#include <QListView>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
#include <gui/mainwindow_p.h>
#pragma execution_character_set("utf-8")

static inline constexpr char InternetText[]{ "请选择要同步的配置" };
static inline constexpr char LocalText[]{ "请选择要备份的配置" };

ConfigSelectWidget::ConfigSelectWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ConfigSelectWidget::~ConfigSelectWidget() { }

void ConfigSelectWidget::initUI()
{
    setStyleSheet(".ConfigSelectWidget{background-color: white; border-radius: 8px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    titileLabel = new QLabel(LocalText, this);
    titileLabel->setFixedHeight(20);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    initSelectBrowerBookMarkFrame();
    initSelectConfigFrame();

    QLabel *tipLabel1 = new QLabel("已扫描系统中的已安装应用，请选择需要迁移的应用", this);
    tipLabel1->setFixedHeight(12);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    determineButton = new QToolButton(this);
    determineButton->setText("确定");
    determineButton->setFixedSize(120, 35);
    determineButton->setStyleSheet(".QToolButton{border-radius: 8px;"
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
    QObject::connect(determineButton, &QToolButton::clicked, this, &ConfigSelectWidget::nextPage);
    QObject::connect(determineButton, &QToolButton::clicked, this,
                     [this]() { emit isOk(SelectItemName::CONFIG, true); });

    cancelButton = new QToolButton(this);
    cancelButton->setText("取消");
    cancelButton->setFixedSize(120, 35);
    cancelButton->setStyleSheet(".QToolButton{border-radius: 8px;"
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

    QObject::connect(cancelButton, &QToolButton::clicked, this, &ConfigSelectWidget::backPage);
    QObject::connect(cancelButton, &QToolButton::clicked, this,
                     [this]() { emit isOk(SelectItemName::CONFIG, false); });

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addSpacing(15);
    buttonLayout->addWidget(determineButton);
    buttonLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(3);
    mainLayout->addWidget(tipLabel1);
    mainLayout->addSpacing(25);
    mainLayout->addWidget(selectBrowerBookMarkFrame);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(selectConfigFrame);
    mainLayout->addLayout(buttonLayout);
}

void ConfigSelectWidget::initSelectBrowerBookMarkFrame()
{
    QVBoxLayout *selectframeLayout = new QVBoxLayout();
    selectframeLayout->setContentsMargins(1, 1, 1, 1);
    ItemTitlebar *titlebar =
            new ItemTitlebar("浏览器书签", "迁移建议", 50, 360, QRectF(10, 12, 16, 16), 3, this);
    titlebar->setFixedSize(500, 36);
    selectBrowerBookMarkFrame = new QFrame(this);
    selectBrowerBookMarkFrame->setFixedSize(500, 190);
    selectBrowerBookMarkFrame->setStyleSheet(".QFrame{"
                                             "border-radius: 8px;"
                                             " border: 1px solid rgba(0,0,0, 0.2);"
                                             " opacity: 1;"
                                             "background-color: rgba(255,255,255, 1);"
                                             "}");
    selectBrowerBookMarkFrame->setLayout(selectframeLayout);

    QStandardItemModel *model = new QStandardItemModel(this);
    browserView = new QListView(this);
    browserView->setStyleSheet(".QListView{"
                               " border: none;"
                               "}");
    browserView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    browserView->setModel(model);
    browserView->setItemDelegate(
            new ItemDelegate(84, 250, 366, 100, 50, QPoint(52, 6), QPoint(10, 9)));
    browserView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    browserView->setSelectionMode(QAbstractItemView::NoSelection);

    QMap<QString, QString> browserList = TransferHelper::instance()->getBrowserList();
    for (auto iterator = browserList.begin(); iterator != browserList.end(); iterator++) {
        ListItem *item = new ListItem();
        item->setData(iterator.key(), Qt::DisplayRole);
        item->setData("是", Qt::ToolTipRole);
        // item->setData(fileinfos[i].filePath(), Qt::UserRole);
        item->setIcon(QIcon(":/icon/app.svg"));
        item->setCheckable(true);
        model->appendRow(item);
    }

    selectframeLayout->addWidget(titlebar);
    selectframeLayout->addWidget(browserView);
}

void ConfigSelectWidget::initSelectConfigFrame()
{
    QVBoxLayout *selectframeLayout = new QVBoxLayout();
    ItemTitlebar *titlebar =
            new ItemTitlebar("个人配置", "迁移建议", 50, 360, QRectF(10, 12, 16, 16), 3, this);
    titlebar->setFixedSize(500, 36);

    selectConfigFrame = new QFrame(this);
    selectframeLayout->setContentsMargins(1, 1, 1, 1);
    selectConfigFrame->setFixedSize(500, 75);
    selectConfigFrame->setStyleSheet(".QFrame{"
                                     "border-radius: 8px;"
                                     "border: 1px solid rgba(0,0,0, 0.2);"
                                     "opacity: 1;"
                                     "background-color: rgba(255,255,255, 1);"
                                     "}");
    selectConfigFrame->setLayout(selectframeLayout);

    QStandardItemModel *model = new QStandardItemModel(this);
    configView = new QListView(this);
    configView->setStyleSheet(".QListView{"
                              " border: none;"
                              "}");
    configView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    configView->setModel(model);
    configView->setItemDelegate(
            new ItemDelegate(84, 250, 366, 100, 50, QPoint(52, 6), QPoint(10, 9)));
    configView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    configView->setSelectionMode(QAbstractItemView::NoSelection);

    ListItem *item = new ListItem();
    item->setData("自定义桌面", Qt::DisplayRole);
    item->setData("是", Qt::ToolTipRole);
    item->setIcon(QIcon(":/icon/app.svg"));
    item->setCheckable(true);
    model->appendRow(item);

    selectframeLayout->addWidget(titlebar);
    selectframeLayout->addWidget(configView);
}

void ConfigSelectWidget::sendOptions()
{
    QStringList browser;
    QAbstractItemModel *model = browserView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, 0);
        QVariant checkboxData = model->data(index, Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(checkboxData.toInt());
        if (checkState == Qt::Checked) {
            browser << model->data(index, Qt::DisplayRole).toString();
        }
    }

    qInfo() << "select browser :" << browser;
    OptionsManager::instance()->addUserOption(Options::kBrowserBookmarks, browser);

    QStringList config;
    model = configView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, 0);
        QVariant checkboxData = model->data(index, Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(checkboxData.toInt());
        if (checkState == Qt::Checked) {
            config << model->data(index, Qt::DisplayRole).toString();
        }
    }

    qInfo() << "select config :" << config;
    OptionsManager::instance()->addUserOption(Options::kConfig, config);
}

void ConfigSelectWidget::changeText()
{
    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        titileLabel->setText(LocalText);
    } else if (method == TransferMethod::kNetworkTransmission) {
        titileLabel->setText(InternetText);
    }
}

void ConfigSelectWidget::nextPage()
{
    // send useroptions
    sendOptions();

    // start
    TransferHelper::instance()->startTransfer();

    // nextpage
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
void ConfigSelectWidget::backPage()
{
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    if (stackedWidget) {
        stackedWidget->setCurrentIndex(PageName::selectmainwidget);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                      "nullptr";
    }
}
