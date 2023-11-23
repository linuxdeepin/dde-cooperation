#include "choosewidget.h"

#include "../select/selectmainwidget.h"
#include "../select/configselectwidget.h"
#include "../select/fileselectwidget.h"
#include "../select/appselectwidget.h"
#include "../getbackup/createbackupfilewidget.h"
#include "../type_defines.h"

#include "../transfer/transferringwidget.h"


#include <utils/optionsmanager.h>
#include <utils/transferhepler.h>
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

ChooseWidget::ChooseWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ChooseWidget::~ChooseWidget() { }

void ChooseWidget::initUI()
{
    setStyleSheet(".ChooseWidget{background-color: white; border-radius: 10px;}");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *textLabel1 = new QLabel(tr("Export to local directory"), this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel1->setFont(font);
    textLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    winItem = new ModeItem(internetMethodName, QIcon(":/icon/select1.png"), this);
    packageItem = new ModeItem(localFileMethodName, QIcon(":/icon/select2.png"), this);

    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->addWidget(winItem, Qt::AlignTop);
    modeLayout->addSpacing(20);
    modeLayout->addWidget(packageItem, Qt::AlignTop);
    modeLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipiconlabel = new QLabel(this);
    tipiconlabel->setPixmap(QIcon(":/icon/warning.svg").pixmap(14, 14));

    QLabel *tiptextlabel = new QLabel(this);
    QString prompt = tr("Unable to connect to the network， please check your network connection or select export to local directory.");
    tiptextlabel->setText(QString("<font size=13px color='#FF5736'>%1</font>").arg(prompt));

    tipiconlabel->setVisible(false);
    tiptextlabel->setVisible(false);

    QHBoxLayout *tiplayout = new QHBoxLayout();
    tiplayout->addWidget(tipiconlabel);
    tiplayout->addSpacing(5);
    tiplayout->addWidget(tiptextlabel);
    tiplayout->setAlignment(Qt::AlignCenter);

    nextButton = new QToolButton(this);
    nextButton->setText(tr("Next"));
    nextButton->setFixedSize(250, 35);
    nextButton->setEnabled(false);
    nextButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                              "background-color: lightgray;"
                              "}");
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(nextButton, Qt::AlignCenter);

    IndexLabel *indelabel = new IndexLabel(0, this);
    indelabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout *indexLayout = new QHBoxLayout();
    indexLayout->addWidget(indelabel, Qt::AlignCenter);

    mainLayout->addSpacing(30);
    mainLayout->addWidget(textLabel1);
    mainLayout->addSpacing(40);
    mainLayout->addLayout(modeLayout);
    mainLayout->addSpacing(90);
    mainLayout->addLayout(tiplayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(indexLayout);

    connect(TransferHelper::instance(), &TransferHelper::onlineStateChanged,
    [this, tipiconlabel, tiptextlabel](bool online) {
        if (online) {
            tipiconlabel->setVisible(false);
            tiptextlabel->setVisible(false);
        } else {
            tipiconlabel->setVisible(true);
            tiptextlabel->setVisible(true);
            winItem->checked = false;
        }
        winItem->setEnable(online);
    });

    connect(nextButton, &QToolButton::clicked, this, &ChooseWidget::nextPage);
    connect(winItem, &ModeItem::clicked, [this ](int state) {
        if (state == true) {
            if (packageItem->checked == true) {
                packageItem->checked = false;
                packageItem->update();
            }
            nextButton->setEnabled(true);
            nextpage = selecPage1;
            transferMethod = TransferMethod::kNetworkTransmission;
        } else {
            nextButton->setEnabled(false);
        }
    });
    connect(packageItem, &ModeItem::clicked, this, [this](int state) {
        if (state == true) {
            if (winItem->checked == true) {
                winItem->checked = false;
                winItem->update();
            }
            nextButton->setEnabled(true);
            nextpage = selecPage2;

            transferMethod = TransferMethod::kLocalExport;
        } else {
            nextButton->setEnabled(false);
        }
    });
}

void ChooseWidget::sendOptions()
{
    QStringList method;
    method << transferMethod;

    qInfo() << "transfer method:" << method;
    OptionsManager::instance()->addUserOption(Options::kTransferMethod, method);
}

void ChooseWidget::nextPage()
{
    clearAllWidget();
    sendOptions();
    changeAllWidgtText();
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());

    if (stackedWidget) {
        stackedWidget->setCurrentIndex(nextpage);
    } else {
        qWarning() << "Jump to next page failed, qobject_cast<QStackedWidget *>(this->parent()) = "
                   "nullptr";
    }
}

void ChooseWidget::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet(".ChooseWidget{ background-color: rgba(255,255,255,1); border-radius: 10px;}");
        nextButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "background-color: lightgray;"
                                  "}");

    } else {
        // dark
        setStyleSheet(".ChooseWidget{background-color: rgba(37, 37, 37,1); border-radius: 10px;}");
        nextButton->setStyleSheet(".QToolButton{border-radius: 8px;"
                                  "opacity: 1;"
                                  "background-color: rgba(255,255,255, 0.1);"
                                  "}");
    }
    winItem->themeChanged(theme);
    packageItem->themeChanged(theme);
}

ModeItem::ModeItem(QString text, QIcon icon, QWidget *parent) : itemText(text), QFrame(parent)
{
    setStyleSheet(".ModeItem{"
                  "border-radius: 8px;"
                  "opacity: 1;"
                  "background-color: rgba(0,0,0, 0.1);}");
    setFixedSize(268, 222);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(icon.pixmap(150, 120));
    iconLabel->setStyleSheet(".QLabel{background-color: rgba(0, 0, 0, 0);}");
    iconLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(iconLabel);
}

ModeItem::~ModeItem() { }

void ModeItem::setEnable(bool able)
{
    enable = able;
    update();
}

void ModeItem::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        setStyleSheet(".ModeItem{"
                      "border-radius: 8px; "
                      "opacity: 1;"
                      "background-color: rgba(0, 0, 0, 0.03);"
                      "}");
        dark = false;
    } else {
        // dark
        setStyleSheet(".ModeItem{"
                      "border-radius: 8px;"
                      "opacity: 1;"
                      "background-color: rgba(255,255,255, 0.1);"
                      "}");
        dark = true;
    }
}

void ModeItem::mousePressEvent(QMouseEvent *event)
{
    if (enable) {
        checked = !checked;
        emit clicked(checked);
        update();
    }
    return QFrame::mousePressEvent(event);
}

void ModeItem::paintEvent(QPaintEvent *event)
{
    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing);
    if (!enable)
        paint.setOpacity(0.6);
    else
        paint.setOpacity(1);
    if (checked) {
        paint.setPen(QPen(QColor(0, 129, 255, 255), 5));
        paint.drawEllipse(12, 12, 16, 16);
    } else {
        paint.setPen(QPen(QColor(65, 77, 104, 255), 1));
        paint.drawEllipse(12, 12, 16, 16);
    }
    QFont font("SourceHanSansSC-Medium");
    font.setPixelSize(14);
    font.setWeight(QFont::Medium);
    font.setStyleName("Normal");
    paint.setFont(font);
    if (dark)
        paint.setPen(QColor(192, 198, 212, 255));
    else
        paint.setPen(QColor(65, 77, 104, 255));
    paint.drawText(36, 24, itemText);
}

void ChooseWidget::changeAllWidgtText()
{
#ifdef _WIN32
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    SelectMainWidget *widgetMainselect =
        qobject_cast<SelectMainWidget *>(stackedWidget->widget(PageName::selectmainwidget));
    ConfigSelectWidget *widgetConfig =
        qobject_cast<ConfigSelectWidget *>(stackedWidget->widget(PageName::configselectwidget));
    AppSelectWidget *widgetApp =
        qobject_cast<AppSelectWidget *>(stackedWidget->widget(PageName::appselectwidget));
    FileSelectWidget *widgetFile =
        qobject_cast<FileSelectWidget *>(stackedWidget->widget(PageName::filewselectidget));
    widgetMainselect->changeText();
    widgetConfig->changeText();
    widgetApp->changeText();
    widgetFile->changeText();
#endif
}

void ChooseWidget::clearAllWidget()
{
#ifdef _WIN32
    OptionsManager::instance()->clear();
    QStackedWidget *stackedWidget = qobject_cast<QStackedWidget *>(this->parent());
    SelectMainWidget *widgetMainselect =

        qobject_cast<SelectMainWidget *>(stackedWidget->widget(PageName::selectmainwidget));
    ConfigSelectWidget *widgetConfig =
        qobject_cast<ConfigSelectWidget *>(stackedWidget->widget(PageName::configselectwidget));
    AppSelectWidget *widgetApp =
        qobject_cast<AppSelectWidget *>(stackedWidget->widget(PageName::appselectwidget));
    FileSelectWidget *widgetFile =
        qobject_cast<FileSelectWidget *>(stackedWidget->widget(PageName::filewselectidget));
    CreateBackupFileWidget *widgetbackupFile =
        qobject_cast<CreateBackupFileWidget *>(stackedWidget->widget(PageName::createbackupfilewidget));
    TransferringWidget *widgetTransfer =   qobject_cast<TransferringWidget *>(stackedWidget->widget(PageName::transferringwidget));

    widgetFile->clear();
    widgetConfig->clear();
    widgetApp->clear();
    widgetMainselect->clear();
    widgetbackupFile->clear();
    widgetTransfer->clear();

#endif
}
