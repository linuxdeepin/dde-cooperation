#include "resultdisplay.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QApplication>
#include <QStandardItemModel>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QStackedWidget>
#include <net/helper/transferhepler.h>

ResultDisplayWidget::ResultDisplayWidget(QWidget *parent)
    : QFrame(parent)
{
    DLOG << "Initializing result display widget";

    initUI();
}

ResultDisplayWidget::~ResultDisplayWidget()
{
    DLOG << "Destroying result display widget";
}

void ResultDisplayWidget::initUI()
{
    DLOG << "Initializing UI";

    setStyleSheet(".ResultDisplayWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    iconLabel->setAlignment(Qt::AlignCenter);

    titileLabel = new QLabel(tr("Transfer completed"), this);
    StyleHelper::setAutoFont(titileLabel, 24, QFont::DemiBold);
    titileLabel->setAlignment(Qt::AlignCenter);

    tiptextlabel = new QLabel(this);
    StyleHelper::setAutoFont(tiptextlabel, 12, QFont::Normal);
    tiptextlabel->setText(QString(tr("Partial information migration failed, please go to UOS for manual transfer")));
    tiptextlabel->setAlignment(Qt::AlignCenter);
    tiptextlabel->setVisible(false);

    resultWindow = new ResultWindow(this);

    QHBoxLayout *textBrowerlayout = new QHBoxLayout();
    textBrowerlayout->setAlignment(Qt::AlignCenter);
    textBrowerlayout->addWidget(resultWindow);

    ButtonLayout *buttonLayout = new ButtonLayout();
    QPushButton *backButton = buttonLayout->getButton1();
    backButton->setText(tr("Back"));
    QPushButton *nextButton = buttonLayout->getButton2();
    nextButton->setText(tr("Exit"));

    connect(backButton, &QPushButton::clicked, this, &ResultDisplayWidget::nextPage);
    connect(nextButton, &QPushButton::clicked, qApp, &QApplication::quit);

    mainLayout->addSpacing(40);
    mainLayout->addWidget(iconLabel);
    mainLayout->addSpacing(5);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(tiptextlabel);
    mainLayout->addLayout(textBrowerlayout);
    mainLayout->addLayout(buttonLayout);

    connect(TransferHelper::instance(), &TransferHelper::addResult, this,
            &ResultDisplayWidget::addResult);
#ifdef __linux__
    connect(TransferHelper::instance(), &TransferHelper::transferFinished, this, [this] {
        TransferHelper::instance()->sendMessage("add_result", processText);
    });
#endif
    DLOG << "Result display widget initialized";
}

void ResultDisplayWidget::nextPage()
{
    DLOG << "Navigating to next page";

    TransferHelper::instance()->sendMessage("change_page", "startTransfer");
    QTimer::singleShot(1000, this, [this] {
        QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
        if (stackedWidget) {
            if (stackedWidget->currentIndex() == PageName::resultwidget) {
                DLOG << "Current index is resultwidget, setting to choosewidget";
                stackedWidget->setCurrentIndex(PageName::choosewidget);
            }
        } else {
            WLOG << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                    "nullptr";
            DLOG << "Stacked widget not found";
        }
        emit TransferHelper::instance()->clearWidget();
    });
}

void ResultDisplayWidget::themeChanged(int theme)
{
    DLOG << "Theme changed to:" << (theme == 1 ? "Light" : "Dark");

    // light
    if (theme == 1) {
        setStyleSheet(".ResultDisplayWidget{background-color: white; border-radius: 10px;}");
    } else {
        // dark
        setStyleSheet(".ResultDisplayWidget{color: rgb(37, 37, 37); border-radius: 10px;}");
    }
    resultWindow->changeTheme(theme);
}

void ResultDisplayWidget::addResult(QString name, bool success, QString reason)
{
    DLOG << "Adding result - Name:" << name.toStdString()
             << "Success:" << success << "Reason:" << reason.toStdString();

    QString info, color;
    if (!success) {
        DLOG << "Transfer not successful, setting status to false";
        setStatus(false);
    }

    resultWindow->updateContent(name, reason, success);
    QString res = success ? "true" : "false";
    processText.append(name + "/" + res + "/" + reason + ";");
}

void ResultDisplayWidget::clear()
{
    DLOG << "Clearing all results";

    resultWindow->clear();
    processText.clear();
    setStatus(true);
}

void ResultDisplayWidget::setStatus(bool success)
{
    DLOG << "Setting status to:" << success;

    tiptextlabel->setVisible(!success);
    if (success) {
        DLOG << "Setting status to success";
        titileLabel->setText(tr("Transfer completed"));
        iconLabel->setPixmap(QIcon(":/icon/success-128.svg").pixmap(96, 96));
    } else {
        DLOG << "Setting status to partial success";
        titileLabel->setText(tr("Transfer completed partially"));
        iconLabel->setPixmap(QIcon(":/icon/success half-96.svg").pixmap(96, 96));
    }
}

ResultWindow::ResultWindow(QFrame *parent)
    : ProcessDetailsWindow(parent)
{
    DLOG << "Initializing result window";
    init();
}

ResultWindow::~ResultWindow()
{
    DLOG << "Destroying result window";
}

void ResultWindow::updateContent(const QString &name, const QString &type, bool success)
{
    DLOG << "Updating content - Name:" << name.toStdString()
             << "Type:" << type.toStdString() << "Success:" << success;

    int maxWith = 430;
    QString nameT = QFontMetrics(StyleHelper::font(3)).elidedText(name, Qt::ElideRight, maxWith);
    QString typeT = QFontMetrics(StyleHelper::font(3)).elidedText(type, Qt::ElideRight, maxWith);

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(this->model());

    for (int col = 0; col < model->columnCount(); ++col) {
        QModelIndex index = model->index(0, col);
        QString itemName = model->data(index, Qt::DisplayRole).toString();
        if (itemName == nameT) {
            DLOG << "Item found, updating ToolTipRole";
            model->setData(index, typeT, Qt::ToolTipRole);
            return;
        }
    }

    QStandardItem *item = new QStandardItem();
    item->setData(nameT, Qt::DisplayRole);
    item->setData(typeT, Qt::ToolTipRole);
    if (success) {
        DLOG << "Setting StatusTipRole to 0 for success";
        item->setData(0, Qt::StatusTipRole);
    } else {
        DLOG << "Setting StatusTipRole to 1 for failure";
        item->setData(1, Qt::StatusTipRole);
    }

    model->appendRow(item);
}

void ResultWindow::changeTheme(int theme)
{
    DLOG << "Changing theme to:" << (theme == 1 ? "Light" : "Dark");

    if (theme == 1) {
        DLOG << "Theme is light, setting stylesheet";
        setStyleSheet(".ResultWindow{background-color: rgba(0, 0, 0, 0.08);"
                      "border-radius: 10px;"
                      "padding: 10px 5px 10px 0px;"
                      "}");
    } else {
        // dark
        DLOG << "Theme is dark, setting stylesheet";
        setStyleSheet(".ResultWindow{background-color: rgba(255,255,255, 0.08);"
                      "border-radius: 10px;"
                      "padding: 10px 5px 10px 0px;"
                      "}");
    }

    ProcessWindowItemDelegate *delegate = qobject_cast<ProcessWindowItemDelegate *>(this->itemDelegate());
    delegate->setTheme(theme);
}

void ResultWindow::init()
{
    DLOG << "Initializing components";

    setStyleSheet(".ResultWindow{background-color: rgba(0, 0, 0, 0.08);"
                  "border-radius: 10px;"
                  "padding: 10px 5px 10px 10px;"
                  "}");
    QStandardItemModel *model = new QStandardItemModel(this);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setModel(model);
    ProcessWindowItemDelegate *delegate = new ProcessWindowItemDelegate();
    delegate->setStageColor(QColor(Qt::red));
    setItemDelegate(delegate);
    setFixedSize(460, 112);
}
