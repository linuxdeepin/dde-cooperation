#include "choosewidget.h"

#include "../select/selectmainwidget.h"
#include "../select/configselectwidget.h"
#include "../select/fileselectwidget.h"
#include "../select/appselectwidget.h"

#include "../type_defines.h"

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

#pragma execution_character_set("utf-8")

inline constexpr char internetMethodName[]{ "从windows PC" };
#ifdef WIN32
inline constexpr char localFileMethodName[]{ "本地导出备份" };
inline constexpr int selecPage1 = PageName::promptwidget;
inline constexpr int selecPage2 = PageName::selectmainwidget;
#else
inline constexpr char localFileMethodName[]{ "从备份文件导入" };
inline constexpr int selecPage1 = PageName::promptwidget;
inline constexpr int selecPage2 = PageName::uploadwidget;
#endif

ChooseWidget::ChooseWidget(QWidget *parent) : QFrame(parent)
{
    initUI();
}

ChooseWidget::~ChooseWidget() { }

void ChooseWidget::initUI()
{
    setStyleSheet("background-color: white; border-radius: 10px;");

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setLayout(mainLayout);
    mainLayout->setSpacing(0);

    QLabel *textLabel1 = new QLabel("选择信息迁移方式", this);
    QFont font;
    font.setPointSize(16);
    font.setWeight(QFont::DemiBold);
    textLabel1->setFont(font);
    textLabel1->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    ModeItem *winItem = new ModeItem(internetMethodName, QIcon(":/icon/select1.png"), this);
    ModeItem *packageItem = new ModeItem(localFileMethodName, QIcon(":/icon/select2.png"), this);

    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->addWidget(winItem, Qt::AlignTop);
    modeLayout->addSpacing(20);
    modeLayout->addWidget(packageItem, Qt::AlignTop);
    modeLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    QLabel *tipiconlabel = new QLabel(this);
    tipiconlabel->setPixmap(QIcon(":/icon/warning.svg").pixmap(14, 14));

    QLabel *tiptextlabel = new QLabel(this);
    tiptextlabel->setText("<font size=13px color='#FF5736' "
                          ">无法连接服务器！请检查网络连接或者选择本地迁移。</font>");

    tipiconlabel->setVisible(false);
    tiptextlabel->setVisible(false);

    QHBoxLayout *tiplayout = new QHBoxLayout(this);
    tiplayout->addWidget(tipiconlabel);
    tiplayout->addSpacing(5);
    tiplayout->addWidget(tiptextlabel);
    tiplayout->setAlignment(Qt::AlignCenter);

    nextButton = new QToolButton(this);
    nextButton->setText("下一步");
    nextButton->setFixedSize(250, 35);
    nextButton->setStyleSheet("background-color: lightgray;");
    nextButton->setEnabled(false);

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
            [winItem, tipiconlabel, tiptextlabel](bool online) {
                if (online) {
                    tipiconlabel->setVisible(false);
                    tiptextlabel->setVisible(false);
                    winItem->setEnable(true);
                } else {
                    tipiconlabel->setVisible(true);
                    tiptextlabel->setVisible(true);
                    winItem->setEnable(false);
                }
            });

    connect(nextButton, &QToolButton::clicked, this, &ChooseWidget::nextPage);
    connect(winItem, &ModeItem::clicked, [this, packageItem](int state) {
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
    connect(packageItem, &ModeItem::clicked, this, [this, winItem](int state) {
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
        setStyleSheet("background-color: white; border-radius: 10px;");
        nextButton->setStyleSheet("background-color: lightgray;");
    } else {
        // dark
        setStyleSheet("background-color: rgb(37, 37, 37); border-radius: 10px;");
        nextButton->setStyleSheet("background-color: rgba(0, 0, 0, 0.08);");
    }
}

ModeItem::ModeItem(QString text, QIcon icon, QWidget *parent) : itemText(text), QFrame(parent)
{
    setStyleSheet(".ModeItem{"
                  "border-radius: 8px;"
                  "opacity: 1;"
                  "background-color: rgba(0,0,0, 0.08);}");
    setFixedSize(268, 222);

    iconLabel = new QLabel(this);
    iconLabel->setPixmap(icon.pixmap(150, 120));
    iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
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
    QPalette palette;
    if (!able) {
        palette.setColor(QPalette::WindowText, Qt::gray);
        setPalette(palette);
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
    paint.setPen(QPen(QColor(65, 77, 104, 255), 1));
    paint.drawEllipse(12, 12, 16, 16);
    if (checked) {
        paint.setBrush(QColor(0, 129, 255, 255));
        paint.drawEllipse(12, 12, 16, 16);

        paint.setBrush(QColor(255, 255, 255, 255));
        paint.drawEllipse(16, 16, 8, 8);
    }


    QFont font("SourceHanSansSC-Medium");
    font.setPixelSize(14);
    font.setWeight(QFont::Medium);
    font.setStyleName("Normal");
    paint.setFont(font);
    paint.setPen(QColor(65, 77, 104,255));
    paint.setRenderHint(QPainter::TextAntialiasing);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setRenderHint(QPainter::SmoothPixmapTransform);

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
