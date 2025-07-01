// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationguihelper.h"
#include "global_defines.h"
#include "common/log.h"

#ifdef linux
#    include <DGuiApplicationHelper>
#    include <DFontSizeManager>
DGUI_USE_NAMESPACE
#endif

#ifdef DTKWIDGET_CLASS_DSizeMode
#    include <DSizeMode>
DWIDGET_USE_NAMESPACE
#endif

#include <QVariant>

using namespace cooperation_core;

CooperationGuiHelper::CooperationGuiHelper(QObject *parent)
    : QObject(parent)
{
    DLOG << "Initializing GUI helper";
    initConnection();
    DLOG << "Initialization completed";
}

void CooperationGuiHelper::initConnection()
{
    DLOG << "Initializing connections";
#ifdef linux
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &CooperationGuiHelper::themeTypeChanged);
    DLOG << "Connected to theme change signals";
#endif
    DLOG << "Connections initialized";
}

CooperationGuiHelper *CooperationGuiHelper::instance()
{
    static CooperationGuiHelper ins;
    return &ins;
}

bool CooperationGuiHelper::autoUpdateTextColor(QWidget *widget, const QList<QColor> &colorList)
{
    DLOG << "Setting up auto text color for widget";
    if (colorList.size() != 2) {
        DLOG << "Invalid color list size:" << colorList.size();
        return false;
    }

    if (isDarkTheme()) {
        DLOG << "Current theme is dark, setting dark color";
        setFontColor(widget, colorList.last());
    } else {
        DLOG << "Current theme is light, setting light color";
        setFontColor(widget, colorList.first());
    }

    if (!widget->property("isConnected").toBool()) {
        DLOG << "Connecting theme change signals for widget";
        widget->setProperty("isConnected", true);
        connect(this, &CooperationGuiHelper::themeTypeChanged, widget, [this, widget, colorList] {
            DLOG << "Theme changed, updating widget text color";
            autoUpdateTextColor(widget, colorList);
        });
    }

    DLOG << "Auto text color setup completed";
    return true;
}

bool CooperationGuiHelper::isDarkTheme()
{
#ifdef linux
    return DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;
#else
    DLOG << "Non-Linux platform, default to light theme";
    return false;
#endif
}

void CooperationGuiHelper::setFontColor(QWidget *widget, QColor color)
{
    QPalette palette = widget->palette();
    palette.setColor(QPalette::WindowText, color);
    widget->setPalette(palette);
    DLOG << "Font color set successfully";
}

void CooperationGuiHelper::setLabelFont(QLabel *label, int pointSize, int minpointSize, int weight)
{
    DLOG << "Setting label font - size:" << pointSize
             << "min:" << minpointSize << "weight:" << weight;
    QFont font;
    int size = pointSize;
#ifdef DTKWIDGET_CLASS_DSizeMode
    size = DSizeModeHelper::element(minpointSize, pointSize);
    DLOG << "Adjusted font size for DSizeMode:" << size;
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::sizeModeChanged, label, [pointSize, minpointSize, label] {
        DLOG << "Size mode changed, updating label font";
        int size = DSizeModeHelper::element(minpointSize, pointSize);
        QFont font;
        font.setPixelSize(size);
        label->setFont(font);
    });
#endif

    font.setPixelSize(size);
    font.setWeight((QFont::Weight)weight);

    label->setFont(font);
    DLOG << "Label font set successfully";
}

void CooperationGuiHelper::setAutoFont(QWidget *widget, int size, int weight)
{
    DLOG << "Setting auto font - size:" << size << "weight:" << weight;
#ifdef linux
    switch (size) {
    case 16:
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T5, weight);
        DLOG << "Using T5 font size";
        break;
    case 14:
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T6, weight);
        DLOG << "Using T6 font size";
        break;
    case 12:
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T8, weight);
        DLOG << "Using T8 font size";
        break;
    case 11:
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T9, weight);
        DLOG << "Using T9 font size";
        break;
    default:
        DFontSizeManager::instance()->bind(widget, DFontSizeManager::T6, weight);
        DLOG << "Using default T6 font size";
    }
#else
    DLOG << "Non-Linux platform, setting font directly";
    QFont font;
    font.setPixelSize(size);
    font.setWeight(weight);
    widget->setFont(font);
#endif
    DLOG << "Auto font set successfully";
}

void CooperationGuiHelper::initThemeTypeConnect(QWidget *w, const QString &lightstyle, const QString &darkstyle)
{
    if (CooperationGuiHelper::instance()->isDarkTheme())
        w->setStyleSheet(darkstyle);
    else
        w->setStyleSheet(lightstyle);
    connect(CooperationGuiHelper::instance(), &CooperationGuiHelper::themeTypeChanged, w, [w, lightstyle, darkstyle] {
        DLOG << "Theme type changed signal received";
        if (CooperationGuiHelper::instance()->isDarkTheme()) {
            DLOG << "Theme changed to dark, applying dark style";
            w->setStyleSheet(darkstyle);
        } else {
            DLOG << "Theme changed to light, applying light style";
            w->setStyleSheet(lightstyle);
        }
    });
    DLOG << "Theme connection initialized";
}
