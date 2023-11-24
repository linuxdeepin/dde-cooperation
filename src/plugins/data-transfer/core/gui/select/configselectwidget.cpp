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

    QLabel *tipLabel1 =
            new QLabel(tr("Check transfer configuration will automatically apply to UOS."), this);
    tipLabel1->setFixedHeight(12);
    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    font.setPointSize(10);
    font.setWeight(QFont::Thin);
    tipLabel1->setFont(font);

    determineButton = new QToolButton(this);
    determineButton->setText(tr("Confirm"));
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
                                   "text-align: center;"
                                   "}");
    QObject::connect(determineButton, &QToolButton::clicked, this, &ConfigSelectWidget::nextPage);

    cancelButton = new QToolButton(this);
    cancelButton->setText(tr("Cancel"));
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
                                "text-align: center;"
                                ";}");

    QObject::connect(cancelButton, &QToolButton::clicked, this, &ConfigSelectWidget::backPage);

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
    ItemTitlebar *titlebar = new ItemTitlebar(tr("Browser bookmarks"), tr("Recommendation"), 50,
                                              360, QRectF(10, 12, 16, 16), 3, this);
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

    browserView = new SelectListView(this);
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(browserView->model());
    browserView->setItemDelegate(
            new ItemDelegate(84, 250, 366, 100, 50, QPoint(52, 6), QPoint(10, 9)));

    QMap<QString, QString> browserList = TransferHelper::instance()->getBrowserList();
    for (auto iterator = browserList.begin(); iterator != browserList.end(); iterator++) {
        QStandardItem *item = new QStandardItem();
        item->setData(iterator.key(), Qt::DisplayRole);
        item->setData(tr("Transferable"), Qt::ToolTipRole);
        item->setIcon(QIcon(iterator.value()));
        item->setCheckable(true);
        model->appendRow(item);
    }

    selectframeLayout->addWidget(titlebar);
    selectframeLayout->addWidget(browserView);
    QObject::connect(titlebar, &ItemTitlebar::selectAll, browserView,
                     &SelectListView::selectorDelAllItem);
}

void ConfigSelectWidget::initSelectConfigFrame()
{
    QVBoxLayout *selectframeLayout = new QVBoxLayout();
    ItemTitlebar *titlebar = new ItemTitlebar(tr("Personal Settings"), tr("Recommendation"), 50,
                                              360, QRectF(10, 12, 16, 16), 3, this);
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

    configView = new SelectListView(this);
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(configView->model());
    configView->setItemDelegate(
            new ItemDelegate(55, 250, 366, 100, 50, QPoint(52, 6), QPoint(10, 9)));

    QStandardItem *item = new QStandardItem();
    item->setData(tr("Customized Wallpaper"), Qt::DisplayRole);
    item->setData(tr("Transferable"), Qt::ToolTipRole);
    item->setCheckable(true);
    model->appendRow(item);

    selectframeLayout->addWidget(titlebar);
    selectframeLayout->addWidget(configView);
    QObject::connect(titlebar, &ItemTitlebar::selectAll, configView,
                     &SelectListView::selectorDelAllItem);
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

    emit isOk(SelectItemName::CONFIG);
}

void ConfigSelectWidget::delOptions()
{
    // Clear All config Selections
    QAbstractItemModel *model = browserView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, 0);
        QVariant checkboxData = model->data(index, Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(checkboxData.toInt());
        if (checkState == Qt::Checked) {
            model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
        }
    }

    model = configView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, 0);
        QVariant checkboxData = model->data(index, Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(checkboxData.toInt());
        if (checkState == Qt::Checked) {
            model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
        }
    }
    OptionsManager::instance()->addUserOption(Options::kBrowserBookmarks, QStringList());
    OptionsManager::instance()->addUserOption(Options::kConfig, QStringList());

    // Deselect
    emit isOk(SelectItemName::CONFIG);
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

void ConfigSelectWidget::clear()
{
    QStandardItemModel *browsermodel = qobject_cast<QStandardItemModel *>(browserView->model());
    for (int row = 0; row < browsermodel->rowCount(); ++row) {
        QModelIndex itemIndex = browsermodel->index(row, 0);
        browsermodel->setData(itemIndex, Qt::Unchecked, Qt::CheckStateRole);
    }

    QStandardItemModel *configmodel = qobject_cast<QStandardItemModel *>(configView->model());
    for (int row = 0; row < configmodel->rowCount(); ++row) {
        QModelIndex itemIndex = configmodel->index(row, 0);
        configmodel->setData(itemIndex, Qt::Unchecked, Qt::CheckStateRole);
    }
    OptionsManager::instance()->addUserOption(Options::kBrowserBookmarks, QStringList());
    OptionsManager::instance()->addUserOption(Options::kConfig, QStringList());
}

void ConfigSelectWidget::nextPage()
{
    // send useroptions
    sendOptions();

    // nextpage
    emit TransferHelper::instance()->changeWidget(PageName::selectmainwidget);
}
void ConfigSelectWidget::backPage()
{
    // delete Options
    delOptions();

    emit TransferHelper::instance()->changeWidget(PageName::selectmainwidget);
}
