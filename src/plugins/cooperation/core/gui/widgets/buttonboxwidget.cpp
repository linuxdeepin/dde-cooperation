// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "buttonboxwidget.h"
#ifdef linux
#include <DIconButton>
#include <DFloatingButton>

DWIDGET_USE_NAMESPACE
#endif

using namespace cooperation_core;

ButtonBoxWidget::ButtonBoxWidget(QWidget *parent)
    : QWidget(parent),
      mainLayout(new QHBoxLayout)
{
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(5);
    setLayout(mainLayout);
}

int ButtonBoxWidget::addButton(const QIcon &icon, const QString &toolTip, ButtonStyle style)
{
    CooperationIconButton *btn { nullptr };
    switch (style) {
    case kNormal:
        btn = new CooperationIconButton(this);
#ifdef linux
        btn->setEnabledCircle(true);
#endif
        break;
    case kHighLight:
        btn = new CooperationFloatingEdit(this);
        break;
    }

    btn->setToolTip(toolTip);
    btn->setFixedSize(32, 32);
    btn->setIconSize({ 16, 16 });
    btn->setIcon(icon);
#ifndef linux
    btn->setStyleSheet(
        "background-color: #0098FF;;"
        "border-radius: 16px;"
        );
#endif
    int index = mainLayout->count();
    mainLayout->addWidget(btn);
    connect(btn, &CooperationIconButton::clicked, this, [this, index] {
        emit this->buttonClicked(index);
    });

    return index;
}

QAbstractButton *ButtonBoxWidget::button(int index)
{
    if (index >= mainLayout->count())
        return nullptr;

    auto item = mainLayout->itemAt(index);
    auto btn = item->widget();

    return qobject_cast<QAbstractButton *>(btn);
}

void ButtonBoxWidget::setButtonVisible(int index, bool visible)
{
    auto btn = button(index);
    if (btn)
        btn->setVisible(visible);
}

void ButtonBoxWidget::setButtonClickable(int index, bool clickable)
{
    auto btn = button(index);
    if (btn)
        btn->setEnabled(clickable);
}

void ButtonBoxWidget::clear()
{
    const int count = mainLayout->count();
    for (int i = 0; i != count; ++i) {
        QLayoutItem *item = mainLayout->takeAt(i);
        QWidget *w = item->widget();
        if (w) {
            w->setParent(nullptr);
            w->deleteLater();
        }

        delete item;
    }
}
