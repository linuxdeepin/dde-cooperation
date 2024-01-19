#include "type_defines.h"

#include <QPainter>

ButtonLayout::ButtonLayout(QWidget *parent)
    : QHBoxLayout(parent)
{
    button1 = new QPushButton(parent);
    button1->setFixedSize(120, 36);

    button2 = new CooperationSuggestButton(parent);
    button2->setFixedSize(120, 36);

#ifdef WIN32
    button1->setStyleSheet(".QPushButton{border-radius: 8px;"
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
    button2->setStyleSheet(".QPushButton{"
                           "border-radius: 8px;"
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
        painter.drawEllipse((diam + 8) * i + 5, 0, diam, diam);
    }
}
