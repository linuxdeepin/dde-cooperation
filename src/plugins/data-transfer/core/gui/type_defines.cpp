#include "type_defines.h"

#include <QPainter>
#include <QStandardItemModel>
#include <QTimer>

#include <QtSvg/QSvgRenderer>

ButtonLayout::ButtonLayout(QWidget *parent)
    : QHBoxLayout(parent)
{
    button1 = new QPushButton(parent);
    button1->setFixedSize(120, 36);

    button2 = new CooperationSuggestButton(parent);
    button2->setFixedSize(120, 36);

#ifdef WIN32
    button1->setStyleSheet(StyleHelper::buttonStyle(StyleHelper::gray));
    button2->setStyleSheet(StyleHelper::buttonStyle(StyleHelper::blue));
#endif
    addWidget(button1);
    addWidget(button2);
    setSpacing(10);
    setAlignment(Qt::AlignCenter);
}

ButtonLayout::~ButtonLayout()
{
}

void ButtonLayout::setCount(int count)
{
    switch (count) {
    case 1:
        button1->setFixedSize(250, 36);
        button2->setVisible(false);
        break;
    case 2:
        button1->setFixedSize(120, 36);
        button2->setVisible(true);
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

void ButtonLayout::themeChanged(int theme)
{
    // light
    if (theme == 1) {
        button1->setStyleSheet(".QToolButton{border-radius: 8px;"
                               "background-color: lightgray;"
                               "}");

    } else {
        // dark
        button1->setStyleSheet(".QToolButton{border-radius: 8px;"
                               "opacity: 1;"
                               "background-color: rgba(255,255,255, 0.1);"
                               "}");
    }
}

QFont StyleHelper::font(int type)
{
    QFont font;
    switch (type) {
    case 1:
        font.setPixelSize(24);
        font.setWeight(QFont::DemiBold);
        break;
    case 2:
        font.setPixelSize(17);
        font.setWeight(QFont::DemiBold);
        break;
    case 3:
        font.setPixelSize(12);
        break;
    default:
        break;
    }
    return font;
}

QString StyleHelper::textStyle(StyleHelper::TextStyle type)
{
    QString style;
    switch (type) {
    case normal:
        style = "color: #000000; font-size: 12px;";
        break;
    case error:
        style = "color: #FF5736; font-size: 12px;";
        break;
    }
    return style;
}

QString StyleHelper::buttonStyle(int type)
{
    QString style;
    switch (type) {
    case gray:
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
    }
    return style;
}

QString StyleHelper::textBrowserStyle(int type)
{
    QString style;
    switch (type) {
    case 1:
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
        if (i == index)
            brushColor.setAlpha(190);
        else
            brushColor.setAlpha(40);

        painter.setBrush(brushColor);
        painter.drawEllipse((diam + 8) * i + 6, 0, diam, diam);
    }
}

MovieWidget::MovieWidget(QString filename, QWidget *parent)
    : QWidget(parent), movie(filename)
{
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
}

ProcessDetailsWindow::~ProcessDetailsWindow() {}

void ProcessDetailsWindow::clear()
{
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
    QSvgRenderer *render = new QSvgRenderer(path);
    renders.push_back(render);
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
        fontNameColor = Qt::white;
        fontStageColor = QColor(123, 159, 191, 255);
    } else {
        fontNameColor = Qt::black;
        fontStageColor = QColor(0, 130, 250, 100);
    }
    if (StatusTipRole != 0) {
        fontStageColor = stageTextColor;
    }

    QFont font;
    font.setPixelSize(12);
    QPen textNamePen(fontNameColor);
    painter->setFont(font);
    painter->setPen(textNamePen);

    QRect remarkTextPos;
    if (!renders.isEmpty()) {
        remarkTextPos = option.rect.adjusted(40, 0, 0, 0);
    } else {
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
    if (renders.isEmpty())
        return;

    painter->save();
    int num = index.data(Qt::UserRole).toInt();
    QPoint pos = option.rect.topLeft();

    QRect iconRect(pos.x() + 10, pos.y(), 20, 20);

    QSvgRenderer *render = renders[num];
    render->render(painter, iconRect);
    painter->restore();
}
