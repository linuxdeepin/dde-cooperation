// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "settingitem.h"
#include "gui/utils/cooperationguihelper.h"
#include "global_defines.h"
#include "common/log.h"

#include <QPainter>
#include <QPainterPath>

using namespace cooperation_core;

SettingItem::SettingItem(QWidget *parent)
    : QFrame(parent)
{
    DLOG << "Initializing setting item";
    mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(10, 6, 10, 6);
    setLayout(mainLayout);
#ifndef linux
    setFixedHeight(48);
#endif
    DLOG << "Initialization completed";
}

void SettingItem::setItemInfo(const QString &text, QWidget *w)
{
    DLOG << "Setting item info with text:" << text.toStdString();
    CooperationLabel *label = new CooperationLabel(text, this);
    auto font = label->font();
    font.setWeight(QFont::Medium);
    label->setFont(font);

    // 设置打点省略显示，使用固定宽度计算
    QFontMetrics fontMetrics(label->font());
    int availableWidth = 300;  // 对话框宽度600px减去左右边距-280
    QString elidedText = fontMetrics.elidedText(text, Qt::ElideRight, availableWidth);
    label->setText(elidedText);
    if (elidedText != text) {
        label->setToolTip(text);  // 如果文本被截断，设置完整文本为工具提示
    }

    mainLayout->addWidget(label, 0, Qt::AlignLeft);
    mainLayout->addWidget(w, 0, Qt::AlignRight);
    DLOG << "Item info set successfully";
}

void SettingItem::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const int radius = 8;
    QRect paintRect = this->rect();
    QPainterPath path;
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
#ifdef linux
    // DLOG << "Linux platform, setting background color";
    QColor color(0, 0, 0, static_cast<int>(255 * 0.03));
#else
    // DLOG << "Non-Linux platform, setting background color";
    QColor color(0, 0, 0, static_cast<int>(255 * 0.09));
#endif
    if (CooperationGuiHelper::isDarkTheme()) {
        // DLOG << "Dark theme detected, adjusting color";
        color.setRgb(255, 255, 255, static_cast<int>(255 * 0.05));
    }

    painter.fillPath(path, color);

    return QFrame::paintEvent(event);
}
