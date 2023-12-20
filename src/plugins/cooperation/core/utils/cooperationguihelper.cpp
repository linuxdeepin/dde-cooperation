// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationguihelper.h"

#ifdef linux
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

#include <QVariant>

using namespace cooperation_core;

CooperationGuiHelper::CooperationGuiHelper(QObject *parent)
    : QObject(parent)
{
    initConnection();
}

void CooperationGuiHelper::initConnection()
{
#ifdef linux
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &CooperationGuiHelper::themeTypeChanged);
#endif
}

CooperationGuiHelper *CooperationGuiHelper::instance()
{
    static CooperationGuiHelper ins;
    return &ins;
}

bool CooperationGuiHelper::autoUpdateTextColor(QWidget *widget, const QList<QColor> &colorList)
{
    if (colorList.size() != 2)
        return false;

    if (isDarkTheme())
        setFontColor(widget, colorList.last());
    else
        setFontColor(widget, colorList.first());

    if (!widget->property("isConnected").toBool()) {
        widget->setProperty("isConnected", true);
        connect(this, &CooperationGuiHelper::themeTypeChanged, widget, [this, widget, colorList] {
            autoUpdateTextColor(widget, colorList);
        });
    }

    return true;
}

bool CooperationGuiHelper::isDarkTheme()
{
#ifdef linux
    return DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;
#else
    return false;
#endif
}

void CooperationGuiHelper::setFontColor(QWidget *widget, QColor color)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::WindowText, color);
    widget->setPalette(palette);
}
