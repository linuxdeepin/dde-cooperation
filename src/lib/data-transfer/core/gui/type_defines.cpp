#include "type_defines.h"
#include "common/log.h"

#include <QPainter>
#include <QStandardItemModel>
#include <QTimer>
#include <QtSvg/QSvgRenderer>

#ifdef linux
#    include <DFontSizeManager>
DWIDGET_USE_NAMESPACE
#endif

ButtonLayout::ButtonLayout(QWidget *parent)
    : QHBoxLayout(parent)
{
    DLOG << "Initializing button layout";

    button1 = new QPushButton(parent);
    button1->setFixedSize(120, 36);

    button2 = new CooperationSuggestButton(parent);
    button2->setFixedSize(120, 36);

#if defined(_WIN32) || defined(_WIN64)
    button1->setStyleSheet(StyleHelper::buttonStyle(StyleHelper::gray));
    button2->setStyleSheet(StyleHelper::buttonStyle(StyleHelper::blue));
#else
    StyleHelper::setAutoFont(button1, 14, QFont::Medium);
    StyleHelper::setAutoFont(button2, 14, QFont::Medium);
#endif
    addWidget(button1);
    addWidget(button2);
    setSpacing(10);
    setAlignment(Qt::AlignCenter);
}

ButtonLayout::~ButtonLayout()
{
    DLOG << "Destroying button layout";
}

void ButtonLayout::setCount(int count)
{
    DLOG << "Setting button count to:" << count;

    switch (count) {
    case 1:
        DLOG << "Setting button count to 1, hiding button2";
        button1->setFixedSize(250, 36);
        button2->setVisible(false);
        break;
    case 2:
        DLOG << "Setting button count to 2, showing button2";
        button1->setFixedSize(120, 36);
        button2->setVisible(true);
        break;
    default:
        DLOG << "Unknown button count:" << count;
        break;
    }
}

QPushButton *ButtonLayout::getButton1() const
{
    return button1;
}

QPushButton *ButtonLayout::getButton2() const
{
    return button2;
}

QFont StyleHelper::font(int type)
{
    QFont font;
    switch (type) {
    case 1:
        DLOG << "Font type 1: pixel size 24, demi-bold";
        font.setPixelSize(24);
        font.setWeight(QFont::DemiBold);
        break;
    case 2:
        DLOG << "Font type 2: pixel size 17, demi-bold";
        font.setPixelSize(17);
        font.setWeight(QFont::DemiBold);
        break;
    case 3:
        DLOG << "Font type 3: pixel size 12";
        font.setPixelSize(12);
        break;
    default:
        DLOG << "Unknown font type:" << type;
        break;
    }
    return font;
}

void StyleHelper::setAutoFont(QWidget *widget, int size, int weight)
{
#ifdef linux
    switch (size) {
    case 54:
        DLOG << "Setting font size 54 (T1)";
        DFontSizeManager::instance()->setFontPixelSize(DFontSizeManager::T1, 54);
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T1, weight);
        break;
    case 24:
        DLOG << "Setting font size 24 (T3)";
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T3, weight);
        break;
    case 17:
        DLOG << "Setting font size 17 (T5)";
        DFontSizeManager::instance()->setFontPixelSize(DFontSizeManager::T5, 17);
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T5, weight);
        break;
    case 14:
        DLOG << "Setting font size 14 (T6)";
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T6, weight);
        break;
    case 12:
        DLOG << "Setting font size 12 (T8)";
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T8, weight);
        break;
    case 11:
        DLOG << "Setting font size 11 (T9)";
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T9, weight);
        break;
    case 10:
        DLOG << "Setting font size 10 (T10)";
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T10, weight);
        break;
    default:
        DLOG << "Setting default font size (T6)";
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T6, weight);
    }
#else
    DLOG << "Non-Linux platform, setting font directly";
    QFont font;
    font.setPixelSize(size);
    font.setWeight(weight);
    widget->setFont(font);
#endif
}

QString StyleHelper::textStyle(StyleHelper::TextStyle type)
{
    QString style;
    switch (type) {
    case normal:
        DLOG << "Text style: normal";
        style = "color: #000000; font-size: 12px;";
        break;
    case error:
        DLOG << "Text style: error";
        style = "color: #FF5736;";
        break;
    default:
        DLOG << "Unknown text style:" << type;
        break;
    }
    return style;
}

QString StyleHelper::buttonStyle(int type)
{
    QString style;
    switch (type) {
    case gray:
        DLOG << "Button style: gray";
        style = ".QPushButton{"
                "border-radius: 8px;"
                "opacity: 1;"
                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                "font-family: \"SourceHanSansSC-Medium\";"
                "font-size: 14px;"
                "font-weight: 500;"
                "color: rgba(65,77,104,1);"
                "font-style: normal;"
                "text-align: center;"
                ";}"
                "QPushButton:disabled {"
                "border-radius: 8px;"
                "opacity: 1;"
                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                "rgba(230, 230, 230, 1), stop:1 rgba(227, 227, 227, 1));"
                "font-family: \"SourceHanSansSC-Medium\";"
                "font-size: 14px;"
                "font-weight: 500;"
                "color: rgba(65,77,104,0.5);"
                "font-style: normal;"
                "text-align: center;"
                "}";
        break;
    case blue:
        DLOG << "Button style: blue";
        style = ".QPushButton{"
                "border-radius: 8px;"
                "opacity: 1;"
                "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 "
                "rgba(37, 183, 255, 1), stop:1 rgba(0, 152, 255, 1));"
                "font-family: \"SourceHanSansSC-Medium\";"
                "font-size: 14px;"
                "font-weight: 500;"
                "color: rgba(255,255,255,1);"
                "font-style: normal;"
                "text-align: center;"
                "}";
        break;
    default:
        DLOG << "Unknown button style:" << type;
        break;
    }
    return style;
}

QString StyleHelper::textBrowserStyle(int type)
{
    QString style;
    switch (type) {
    case 1:
        DLOG << "Text browser style: type 1 (light)";
        style = "QTextBrowser {"
                "border-radius: 10px;"
                "padding-top: 10px;"
                "padding-bottom: 10px;"
                "padding-left: 5px;"
                "padding-right: 5px;"
                "font-size: 12px;"
                "font-weight: 400;"
                "color: rgb(82, 106, 127);"
                "line-height: 300%;"
                "background-color:rgba(0, 0, 0,0.08);}";
        break;
    case 0:
        DLOG << "Text browser style: type 0 (dark)";
        style = "QTextBrowser {"
                "border-radius: 10px;"
                "padding-top: 10px;"
                "padding-bottom: 10px;"
                "padding-left: 5px;"
                "padding-right: 5px;"
                "font-size: 12px;"
                "font-weight: 400;"
                "color: rgb(82, 106, 127);"
                "line-height: 300%;"
                "background-color:rgba(255,255,255, 0.1);}";
        break;
    default:
        DLOG << "Unknown text browser style type:" << type;
        break;
    }
    return style;
}

IndexLabel::IndexLabel(int index, QWidget *parent)
    : QLabel(parent), index(index)
{
    setFixedSize(60, 10);
}

void IndexLabel::setIndex(int i)
{
    index = i;
    update();
}

void IndexLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    int diam = 6;

    QColor brushColor;
    brushColor.setNamedColor("#0081FF");
    for (int i = 0; i < 4; i++) {
        if (i == index) {
            // DLOG << "Setting brush color alpha to 190 for index:" << i;
            brushColor.setAlpha(190);
        } else {
            // DLOG << "Setting brush color alpha to 40 for index:" << i;
            brushColor.setAlpha(40);
        }

        painter.setBrush(brushColor);
        painter.drawEllipse((diam + 8) * i + 6, 0, diam, diam);
    }
}

MovieWidget::MovieWidget(QString filename, QWidget *parent)
    : QWidget(parent), movie(filename)
{
    DLOG << "[MovieWidget] Creating movie widget with file:" << filename.toStdString();

    setFixedSize(200, 160);
    loadFrames();
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MovieWidget::nextFrame);
    timer->start(50);
}

void MovieWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0, 0, frames[currentFrame]);
}

void MovieWidget::nextFrame()
{
    currentFrame = (currentFrame + 1) % frames.size();
    update();
}

void MovieWidget::loadFrames()
{
    for (int i = 0; i <= 49; ++i) {
        QPixmap frame = QIcon(":/icon/movie/" + movie + "/" + movie + QString::number(i) + ".png")
                                .pixmap(200, 160);
        frames.append(frame);
    }
}

ProcessDetailsWindow::ProcessDetailsWindow(QFrame *parent)
    : QListView(parent)
{
    DLOG << "[ProcessDetailsWindow] Initializing process details window";
}

ProcessDetailsWindow::~ProcessDetailsWindow() {}

void ProcessDetailsWindow::clear()
{
    DLOG << "[ProcessDetailsWindow] Clearing process details";

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(this->model());
    model->clear();
}

ProcessWindowItemDelegate::ProcessWindowItemDelegate()
{
}

ProcessWindowItemDelegate::~ProcessWindowItemDelegate() {}

void ProcessWindowItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    paintIcon(painter, option, index);
    paintText(painter, option, index);
}

QSize ProcessWindowItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(100, 20);
}

void ProcessWindowItemDelegate::setTheme(int newTheme)
{
    theme = newTheme;
}

void ProcessWindowItemDelegate::addIcon(const QString &path)
{
    QPixmap pixmap = QIcon(path).pixmap(20, 20);
    pixmaps.push_back(pixmap);
}

void ProcessWindowItemDelegate::setStageColor(QColor color)
{
    stageTextColor = color;
}

void ProcessWindowItemDelegate::paintText(QPainter *painter, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    painter->save();
    QString processName = index.data(Qt::DisplayRole).toString();
    QString processStage = index.data(Qt::ToolTipRole).toString();
    int StatusTipRole = index.data(Qt::StatusTipRole).toInt();
    QColor fontNameColor;
    QColor fontStageColor;
    if (theme == 0) {
        // DLOG << "Theme is dark, setting font colors for dark theme";
        fontNameColor = QColor(255, 255, 255, 155);
        fontStageColor = QColor(123, 159, 191, 255);
    } else {
        // DLOG << "Theme is light, setting font colors for light theme";
        fontNameColor = QColor(0, 0, 0, 155);
        fontStageColor = QColor(0, 130, 250, 100);
    }
    if (StatusTipRole != 0) {
        // DLOG << "StatusTipRole is not 0, setting fontStageColor to stageTextColor";
        fontStageColor = stageTextColor;
    }
#ifdef linux
    QFont font(qApp->font());
#else
    QFont font;
    font.setPixelSize(14);
#endif
    font.setPixelSize(QFontInfo(font).pixelSize() - 2);
    QPen textNamePen(fontNameColor);
    painter->setFont(font);
    painter->setPen(textNamePen);

    QRect remarkTextPos;
    if (!pixmaps.isEmpty()) {
        // DLOG << "Pixmaps is not empty, adjusting remarkTextPos";
        remarkTextPos = option.rect.adjusted(40, 0, 0, 0);
    } else {
        // DLOG << "Pixmaps is empty, adjusting remarkTextPos";
        remarkTextPos = option.rect.adjusted(20, 0, 0, 0);
    }
    painter->drawText(remarkTextPos, Qt::AlignLeft | Qt::AlignVCenter, processName);

    QFontMetrics fontMetrics(font);
    int firstTextWidth = fontMetrics.horizontalAdvance(processName);

    QPen textStagePen(fontStageColor);
    painter->setPen(textStagePen);
    remarkTextPos.adjust(firstTextWidth + 20, 0, 0, 0);
    painter->drawText(remarkTextPos, Qt::AlignLeft | Qt::AlignVCenter, processStage);

    painter->restore();
}

void ProcessWindowItemDelegate::paintIcon(QPainter *painter, const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    if (pixmaps.isEmpty()) {
        // DLOG << "Pixmaps is empty, returning";
        return;
    }

    painter->save();
    int num = index.data(Qt::UserRole).toInt();
    QPoint pos = option.rect.topLeft();
    QRect iconRect(pos.x() + 10, pos.y(), 20, 20);
    QPixmap pixmap = pixmaps[num];
    painter->drawPixmap(iconRect, pixmap);
    painter->restore();
}
