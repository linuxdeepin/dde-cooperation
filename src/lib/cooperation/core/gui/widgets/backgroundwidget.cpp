// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backgroundwidget.h"
#include "common/log.h"

#ifdef linux
#include <DGuiApplicationHelper>
#endif

#include <QPainter>
#include <QPainterPath>

BackgroundWidget::BackgroundWidget(QWidget *parent)
    : QFrame(parent)
{
    DLOG << "Initializing background widget";
}

void BackgroundWidget::setBackground(int radius, ColorType colorType, RoundRole role)
{
    this->radius = radius;
    this->colorType = colorType;
    roundRole = role;
    DLOG << "Background settings applied";
}

void BackgroundWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    QRect paintRect = this->rect();
    QPainterPath path;

    switch (roundRole) {
    case Top:
        // DLOG << "Painting top rounded background";
        path.moveTo(paintRect.bottomRight());
        path.lineTo(paintRect.topRight() + QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)),
                         QSize(radius * 2, radius * 2)),
                   0, 90);
        path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
        path.lineTo(paintRect.bottomLeft());
        path.lineTo(paintRect.bottomRight());
        break;
    case Bottom:
        // DLOG << "Painting bottom rounded background";
        path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
        path.lineTo(paintRect.topRight());
        path.lineTo(paintRect.topLeft());
        path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)),
                         QSize(radius * 2, radius * 2)),
                   180, 90);
        path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)),
                         QSize(radius * 2, radius * 2)),
                   270, 90);
        break;
    case TopAndBottom:
        // DLOG << "Painting top and bottom rounded background";
        path.moveTo(paintRect.bottomRight() - QPoint(0, radius));
        path.lineTo(paintRect.topRight() + QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.topRight() - QPoint(radius * 2, 0)),
                         QSize(radius * 2, radius * 2)),
                   0, 90);
        path.lineTo(paintRect.topLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.topLeft()), QSize(radius * 2, radius * 2)), 90, 90);
        path.lineTo(paintRect.bottomLeft() - QPoint(0, radius));
        path.arcTo(QRect(QPoint(paintRect.bottomLeft() - QPoint(0, radius * 2)),
                         QSize(radius * 2, radius * 2)),
                   180, 90);
        path.lineTo(paintRect.bottomLeft() + QPoint(radius, 0));
        path.arcTo(QRect(QPoint(paintRect.bottomRight() - QPoint(radius * 2, radius * 2)),
                         QSize(radius * 2, radius * 2)),
                   270, 90);
        break;
    default:
        // DLOG << "Painting default background";
        break;
    }

    painter.fillPath(path, backgroundColor());
    QFrame::paintEvent(event);
}

QColor BackgroundWidget::backgroundColor()
{
    QColor color(255, 255, 255);
    switch (colorType) {
    case ItemBackground:
        // DLOG << "Color type is ItemBackground";
        color.setRgb(0, 0, 0, static_cast<int>(255 * 0.05));
#ifdef linux
        if (DTK_GUI_NAMESPACE::DGuiApplicationHelper::instance()->themeType() == DTK_GUI_NAMESPACE::DGuiApplicationHelper::DarkType) {
            // DLOG << "Linux dark theme detected for ItemBackground";
            color.setRgb(255, 255, 255, static_cast<int>(255 * 0.05));
        }
#endif
        break;
    default:
#ifdef linux
        if (DTK_GUI_NAMESPACE::DGuiApplicationHelper::instance()->themeType() == DTK_GUI_NAMESPACE::DGuiApplicationHelper::DarkType) {
            // DLOG << "Linux dark theme detected for default background";
            color.setRgb(255, 255, 255, static_cast<int>(255 * 0.03));
        }
#endif
        break;
    }

    return color;
}
