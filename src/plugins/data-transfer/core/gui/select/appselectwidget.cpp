#include "appselectwidget.h"
#include "../type_defines.h"
#include "../win/drapwindowsdata.h"
#include "item.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QListView>

#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
#include <gui/mainwindow_p.h>

AppSelectWidget::AppSelectWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

AppSelectWidget::~AppSelectWidget() { }

void AppSelectWidget::initUI()
{
    setStyleSheet(".AppSelectWidget{background-color: white; border-radius: 8px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    titileLabel = new QLabel(LocalText, this);
    titileLabel->setFixedHeight(20);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    titileLabel->setFont(font);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    initSelectFrame();

    QLabel *tipLabel1 = new QLabel(tr("Check transfer application will automatically install the corresponding UOS version of the application."),
                                   this);
    tipLabel1->setWordWrap(true);
    tipLabel1->setFixedHeight(30);

    tipLabel1->setAlignment(Qt::AlignTop | Qt::AlignCenter);
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
    QObject::connect(determineButton, &QToolButton::clicked, this, &AppSelectWidget::nextPage);

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

    QObject::connect(cancelButton, &QToolButton::clicked, this, &AppSelectWidget::backPage);

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
    mainLayout->addSpacing(10);
    mainLayout->addWidget(selectFrame);

    mainLayout->addLayout(buttonLayout);
}

void AppSelectWidget::initSelectFrame()
{
    QVBoxLayout *selectframeLayout = new QVBoxLayout();
    selectframeLayout->setContentsMargins(1, 1, 1, 1);
    ItemTitlebar *titlebar = new ItemTitlebar(tr("Application"), tr("Recommendation"), 50, 360,
                                              QRectF(10, 12, 16, 16), 3, this);
    titlebar->setFixedSize(500, 36);

    selectFrame = new QFrame(this);

    selectFrame->setFixedSize(500, 318);
    selectFrame->setProperty("class", "myselectframe");
    selectFrame->setStyleSheet(".myselectframe{"
                               "border-radius: 8px;"
                               " border: 1px solid rgba(0,0,0, 0.2);"
                               " opacity: 1;"
                               "background-color: rgba(255,255,255, 1);"
                               "}");
    selectFrame->setLayout(selectframeLayout);

    appView = new SelectListView(this);
    appView->setVerticalScrollMode(QListView::ScrollPerPixel);
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(appView->model());
    appView->setItemDelegate(new ItemDelegate(84, 250, 366, 100, 50, QPoint(52, 6), QPoint(10, 9)));

    QMap<QString, QString> noRecommendList;
    QMap<QString, QString> appList = TransferHelper::instance()->getAppList(noRecommendList);
    for (auto iterator = appList.begin(); iterator != appList.end(); ++iterator) {
        QStandardItem *item = new QStandardItem();
        item->setData(iterator.key(), Qt::DisplayRole);
        item->setData(tr("Transferable"), Qt::ToolTipRole);
        item->setIcon(QIcon(iterator.value()));
        item->setCheckable(true);
        model->appendRow(item);
    }

    for (auto iterator = noRecommendList.begin(); iterator != noRecommendList.end(); ++iterator) {
        QStandardItem *item = new QStandardItem();
        item->setData(iterator.key(), Qt::DisplayRole);
        item->setData(tr("Not Suitable"), Qt::ToolTipRole);
        item->setData(true, Qt::BackgroundRole);
        QPixmap pix = DrapWindowsData::instance()->getAppIcon(iterator.value());
        if (pix.isNull()) {
            item->setIcon(QIcon(":/icon/fileicon.svg"));
        } else {
            item->setIcon(QIcon(pix));
        }

        item->setCheckable(false);

        model->appendRow(item);
    }

    selectframeLayout->addWidget(titlebar);
    selectframeLayout->addWidget(appView);

    QObject::connect(titlebar, &ItemTitlebar::selectAll, appView,
                     &SelectListView::selectorDelAllItem);
}

void AppSelectWidget::changeText()
{
    QString method = OptionsManager::instance()->getUserOption(Options::kTransferMethod)[0];
    if (method == TransferMethod::kLocalExport) {
        titileLabel->setText(LocalText);
    } else if (method == TransferMethod::kNetworkTransmission) {
        titileLabel->setText(InternetText);
    }
}

void AppSelectWidget::clear()
{
    QStandardItemModel *configmodel = qobject_cast<QStandardItemModel *>(appView->model());
    for (int row = 0; row < configmodel->rowCount(); ++row) {
        QModelIndex itemIndex = configmodel->index(row, 0);
        configmodel->setData(itemIndex, Qt::Unchecked, Qt::CheckStateRole);
    }
      OptionsManager::instance()->addUserOption(Options::kApp,QStringList());
}

void AppSelectWidget::sendOptions()
{
    QStringList appName;
    QAbstractItemModel *model = appView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, 0);
        QVariant checkboxData = model->data(index, Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(checkboxData.toInt());
        if (checkState == Qt::Checked) {
            appName << model->data(index, Qt::DisplayRole).toString();
        }
    }

    qInfo() << "select app :" << appName;
    OptionsManager::instance()->addUserOption(Options::kApp, appName);

    emit isOk(SelectItemName::APP);
}

void AppSelectWidget::delOptions()
{
    // Clear All App Selections
    QAbstractItemModel *model = appView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex index = model->index(row, 0);
        QVariant checkboxData = model->data(index, Qt::CheckStateRole);
        Qt::CheckState checkState = static_cast<Qt::CheckState>(checkboxData.toInt());
        if (checkState == Qt::Checked) {
            model->setData(index, Qt::Unchecked, Qt::CheckStateRole);
        }
    }
    OptionsManager::instance()->addUserOption(Options::kApp, QStringList());

    // Deselect
    emit isOk(SelectItemName::APP);
}

void AppSelectWidget::nextPage()
{
    // send useroptions
    sendOptions();

    // nextpage
    emit TransferHelper::instance()->changeWidget(PageName::selectmainwidget);
}
void AppSelectWidget::backPage()
{
    // delete Options
    delOptions();
    //backpage
    emit TransferHelper::instance()->changeWidget(PageName::selectmainwidget);
}
