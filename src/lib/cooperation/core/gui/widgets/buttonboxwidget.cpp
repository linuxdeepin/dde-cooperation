// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "buttonboxwidget.h"
#include "common/log.h"

using namespace cooperation_core;

ButtonBoxWidget::ButtonBoxWidget(QWidget *parent)
    : QWidget(parent),
      mainLayout(new QHBoxLayout)
{
    DLOG << "Initializing button box";
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);
    setLayout(mainLayout);
    DLOG << "Initialization completed";
}

int ButtonBoxWidget::addButton(const QIcon &icon, const QString &toolTip, ButtonStyle style)
{
    CooperationIconButton *btn = new CooperationIconButton(this);
    switch (style) {
    case kNormal:
        DLOG << "Button style: Normal";
#ifndef linux
        btn->setStyleSheet(
                    ".QToolButton {"
                    "background-color: rgba(0,0,0,0.1);"
                    "border-radius: 16px;"
                    "}"
                    "QToolTip {"
                    "background-color: white;"
                    "}");
#endif
        break;
    case kHighLight:
        DLOG << "Button style: Highlight";
#ifdef linux
        btn->setBackgroundRole(QPalette::Highlight);
#else
        btn->setStyleSheet(
            ".QToolButton {"
            "background-color: #0098FF;"
            "border-radius: 16px;"
            "}"
            "QToolTip {"
            "background-color: white;"
            "}");
#endif
        break;
    }

#ifdef linux
    btn->setEnabledCircle(true);
#endif
    btn->setToolTip(toolTip);
    btn->setFixedSize(32, 32);
    btn->setIconSize({ 16, 16 });
    btn->setIcon(icon);

    int index = mainLayout->count();
    mainLayout->addWidget(btn);
    connect(btn, &CooperationIconButton::clicked, this, [this, index] {
        emit this->buttonClicked(index);
    });

    DLOG << "Button added at index:" << index;
    return index;
}

QAbstractButton *ButtonBoxWidget::button(int index)
{
    if (index >= mainLayout->count())
        return nullptr;

    auto item = mainLayout->itemAt(index);
    auto btn = item->widget();

    // DLOG << "Button retrieved successfully";
    return qobject_cast<QAbstractButton *>(btn);
}

void ButtonBoxWidget::setButtonVisible(int index, bool visible)
{
    // DLOG << "Setting button visibility at index:" << index << " to:" << visible;
    auto btn = button(index);
    if (btn) {
        btn->setVisible(visible);
        // DLOG << "Button visibility set successfully";
    } else {
        WLOG << "Button not found at index:" << index;
    }
}

void ButtonBoxWidget::setButtonClickable(int index, bool clickable)
{
    DLOG << "Setting button clickable at index:" << index << "to:" << clickable;
    auto btn = button(index);
    if (btn) {
        btn->setEnabled(clickable);
        DLOG << "Button clickable state set successfully";
    } else {
        WLOG << "Button not found at index:" << index;
    }
}

void ButtonBoxWidget::clear()
{
    DLOG << "Clearing all buttons";
    const int count = mainLayout->count();
    DLOG << "Removing" << count << "buttons";
    for (int i = 0; i != count; ++i) {
        QLayoutItem *item = mainLayout->takeAt(i);
        QWidget *w = item->widget();
        if (w) {
            w->setParent(nullptr);
            w->deleteLater();
            DLOG << "Button at index:" << i << "removed";
        }

        delete item;
    }
    DLOG << "All buttons cleared";
}
