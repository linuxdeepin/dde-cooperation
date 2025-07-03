#include "choosewidget.h"

#include "../select/selectmainwidget.h"
#include "../select/configselectwidget.h"
#include "../select/fileselectwidget.h"
#include "../select/appselectwidget.h"
#include "../getbackup/createbackupfilewidget.h"
#include "../type_defines.h"

#include "../transfer/transferringwidget.h"

#include <utils/optionsmanager.h>
#include <net/helper/transferhepler.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QToolButton>
#include <QStackedWidget>
#include <QCheckBox>
#include <QTextBrowser>
#include <QToolTip>
#include <QImage>
#include <QMessageBox>

ChooseWidget::ChooseWidget(QWidget *parent)
    : QFrame(parent)
{
    DLOG << "Widget constructor called";
    initUI();
}

ChooseWidget::~ChooseWidget()
{
    DLOG << "Widget destructor called";
}

void ChooseWidget::initUI()
{
    DLOG << "ChooseWidget initUI";
    setStyleSheet(".ChooseWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *titileLabel = new QLabel(tr("Select a transfer way"), this);
    StyleHelper::setAutoFont(titileLabel, 24, QFont::DemiBold);
    titileLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    winItem = new ModeItem(internetMethodName, QIcon(":/icon/select1.png"), this);
    packageItem = new ModeItem(localFileMethodName, QIcon(":/icon/select2.png"), this);

    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->addWidget(winItem, Qt::AlignTop);
    modeLayout->addSpacing(20);
    modeLayout->addWidget(packageItem, Qt::AlignTop);
    modeLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tiptextlabel = new QLabel(this);
    tiptextlabel->setStyleSheet(StyleHelper::textStyle(StyleHelper::error));
    QString prompt = tr("Unable to connect to the network， please check your network connection or select export to local directory.");
    tiptextlabel->setText(prompt);

    tiptextlabel->setVisible(false);

    QHBoxLayout *tiplayout = new QHBoxLayout();
    tiplayout->addSpacing(5);
    tiplayout->addWidget(tiptextlabel);
    tiplayout->setAlignment(Qt::AlignCenter);

    ButtonLayout *buttonLayout = new ButtonLayout();
    buttonLayout->setCount(1);
    nextButton = buttonLayout->getButton1();
    nextButton->setText(tr("Next"));
    nextButton->setEnabled(false);

    IndexLabel *indelabel = new IndexLabel(0, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addSpacing(40);
    mainLayout->addWidget(titileLabel);
    mainLayout->addSpacing(60);
    mainLayout->addLayout(modeLayout);
    mainLayout->addSpacing(90);
    mainLayout->addLayout(tiplayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);

    connect(TransferHelper::instance(), &TransferHelper::onlineStateChanged,
            [this, tiptextlabel](bool online) {
                DLOG << "Online state changed:" << online;
                if (online) {
                    DLOG << "Network is online";
                    tiptextlabel->setVisible(false);
                    winItem->setIcon(QIcon(":/icon/select1.png"));
                    nextButton->setEnabled(true);
                } else {
                    DLOG << "Network is offline";
                    tiptextlabel->setVisible(true);
                    winItem->checked = false;
                    nextButton->setEnabled(false);
                }
                winItem->setEnable(online);
            });

    connect(nextButton, &QToolButton::clicked, this, &ChooseWidget::nextPage);
    connect(winItem, &ModeItem::clicked, [this](int state) {
        if (state == true) {
            DLOG << "User selected network transmission mode";
            if (packageItem->checked == true) {
                DLOG << "Package item was checked, unchecking";
                packageItem->checked = false;
                packageItem->update();
            }
            nextButton->setEnabled(true);
            nextpage = selecPage1;
            transferMethod = TransferMethod::kNetworkTransmission;
        } else {
            DLOG << "User deselected network transmission mode";
            nextButton->setEnabled(false);
        }
    });
    connect(packageItem, &ModeItem::clicked, this, [this](int state) {
        if (state == true) {
            DLOG << "User selected local export mode";
            if (winItem->checked == true) {
                DLOG << "Win item was checked, unchecking";
                winItem->checked = false;
                winItem->update();
            }
            nextButton->setEnabled(true);
            nextpage = selecPage2;

            transferMethod = TransferMethod::kLocalExport;
        } else {
            DLOG << "User deselected local export mode";
            nextButton->setEnabled(false);
        }
    });
    DLOG << "ChooseWidget initUI finished";
}

void ChooseWidget::sendOptions()
{
    DLOG << "ChooseWidget sendOptions";
    QStringList method;
    method << transferMethod;

    OptionsManager::instance()->addUserOption(Options::kTransferMethod, method);
    DLOG << "ChooseWidget sendOptions finished";
}

void ChooseWidget::nextPage()
{
    DLOG << "ChooseWidget nextPage";
    sendOptions();
    emit TransferHelper::instance()->changeWidgetText();
    emit TransferHelper::instance()->changeWidget((PageName)nextpage);
    DLOG << "ChooseWidget nextPage finished";
}

void ChooseWidget::themeChanged(int theme)
{
    DLOG << "Theme changed to:" << (theme == 1 ? "light" : "dark");

    // light
    if (theme == 1) {
        DLOG << "Theme is light, setting stylesheet";
        setStyleSheet(".ChooseWidget{ background-color: rgba(255,255,255,1); border-radius: 10px;}");
    } else {
        // dark
        DLOG << "Theme is dark, setting stylesheet";
        setStyleSheet(".ChooseWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
    }
    winItem->themeChanged(theme);
    packageItem->themeChanged(theme);
    DLOG << "ChooseWidget themeChanged finished";
}

ModeItem::ModeItem(QString text, QIcon icon, QWidget *parent)
    : itemText(text), QFrame(parent)
{
    DLOG << "ModeItem constructor called with text:" << text.toStdString();
    setStyleSheet(".ModeItem{"
                  "border-radius: 8px;"
                  "opacity: 1;"
                  "background-color: rgba(0,0,0, 0.1);}"
                  ".ModeItem:hover{"
                  "background-color: rgba(0,0,0, 0.2);}");
    setFixedSize(268, 222);

    StyleHelper::setAutoFont(this, 14, QFont::Medium);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(icon.pixmap(150, 120));
    iconLabel->setStyleSheet(".QLabel{background-color: rgba(0, 0, 0, 0);}");
    iconLabel->setAlignment(Qt::AlignCenter);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(iconLabel);
    DLOG << "ModeItem constructor finished";
}

ModeItem::~ModeItem() {}

void ModeItem::setEnable(bool able)
{
    enable = able;
#ifdef linux
    DLOG << "Linux platform, setting enabled state:" << able;
    setEnabled(able);
#else
    DLOG << "Non-Linux platform, setting stylesheet based on enabled state:" << able;
    if (able)
        setStyleSheet(".ModeItem{"
                      "border-radius: 8px;"
                      "opacity: 1;"
                      "background-color: rgba(0,0,0, 0.1);}"
                      ".ModeItem:hover{"
                      "background-color: rgba(0,0,0, 0.2);}");
    else
        setStyleSheet(".ModeItem{"
                      "border-radius: 8px;"
                      "opacity: 1;"
                      "background-color: rgba(0,0,0, 0.1);}");
#endif
    update();
}

void ModeItem::setIcon(QIcon icon)
{
    iconLabel->setPixmap(icon.pixmap(150, 120));
    update();
}

void ModeItem::themeChanged(int theme)
{
    DLOG << "ModeItem themeChanged called with theme:" << theme;
    // light
    if (theme == 1) {
        DLOG << "Theme is light, setting stylesheet";
        setStyleSheet(".ModeItem{"
                      "border-radius: 8px;"
                      "opacity: 1;"
                      "background-color: rgba(0,0,0, 0.1);}"
                      ".ModeItem:hover{"
                      "background-color: rgba(0,0,0, 0.2);}");
        dark = false;
    } else {
        // dark
        DLOG << "Theme is dark, setting stylesheet";
        setStyleSheet(".ModeItem{"
                      "border-radius: 8px;"
                      "opacity: 1;"
                      "background-color: rgba(255,255,255, 0.1);}"
                      ".ModeItem:hover{"
                      "background-color: rgba(0,0,0, 0.2);}");
        dark = true;
    }
}

void ModeItem::mousePressEvent(QMouseEvent *event)
{
    if (enable) {
        DLOG << "Mouse pressed and item is enabled";
        checked = !checked;
        emit clicked(checked);
        update();
    } else {
        DLOG << "Mouse pressed but item is disabled";
    }
    return QFrame::mousePressEvent(event);
}

void ModeItem::paintEvent(QPaintEvent *event)
{
    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing);
    if (!enable) {
        // DLOG << "Item is disabled, setting opacity to 0.4";
        paint.setOpacity(0.4);
    } else {
        // DLOG << "Item is enabled, setting opacity to 1";
        paint.setOpacity(1);
    }

    if (checked) {
        // DLOG << "Item is checked, drawing blue ellipse";
        paint.setPen(QPen(QColor(0, 129, 255, 255), 5));
        paint.drawEllipse(12, 12, 16, 16);
    } else {
        // DLOG << "Item is not checked, drawing gray ellipse";
        paint.setPen(QPen(QColor(65, 77, 104, 255), 1));
        paint.drawEllipse(12, 12, 16, 16);
    }
    if (dark) {
        // DLOG << "Theme is dark, setting pen color to light gray";
        paint.setPen(QColor(192, 198, 212, 255));
    } else {
        // DLOG << "Theme is light, setting pen color to dark gray";
        paint.setPen(QColor(65, 77, 104, 255));
    }
    paint.drawText(36, 24, itemText);
    return QFrame::paintEvent(event);
}
