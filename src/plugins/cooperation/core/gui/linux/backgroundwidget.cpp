// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "backgroundwidget.h"

#include <DStyle>

using namespace cooperation_core;
DWIDGET_USE_NAMESPACE

BackgroundWidget::BackgroundWidget(QWidget *parent)
    : QFrame(parent)
{
}

void BackgroundWidget::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);

    //设置m_bgGroup 的大小
    if (bgGroup)
        bgGroup->setFixedSize(size());
}

void BackgroundWidget::setBackground(int radius, ColorType colorType)
{
    //加入一个 DFrame 作为圆角背景
    if (bgGroup)
        bgGroup->deleteLater();
    bgGroup = new DFrame(this);
    bgGroup->setBackgroundRole(static_cast<DPalette::ColorType>(colorType));
    bgGroup->setLineWidth(0);
    DStyle::setFrameRadius(bgGroup, radius);

    //将 m_bgGroup 沉底
    bgGroup->lower();
    //设置m_bgGroup 的大小
    bgGroup->setFixedSize(size());
}
