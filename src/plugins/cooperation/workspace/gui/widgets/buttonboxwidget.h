// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUTTONBOXWIDGET_H
#define BUTTONBOXWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QAbstractButton>

namespace cooperation_workspace {

class ButtonBoxWidget : public QWidget
{
    Q_OBJECT
public:
    enum ButtonStyle {
        kNormal,
        kHighLight
    };

    explicit ButtonBoxWidget(QWidget *parent = nullptr);

    int addButton(const QIcon &icon, const QString &toolTip, ButtonStyle style = kNormal);
    QAbstractButton *button(int index);
    void setButtonVisible(int index, bool visible);
    void setButtonClickable(int index, bool clickable);

    void clear();

Q_SIGNALS:
    void buttonClicked(int index);

private:
    QHBoxLayout *mainLayout { nullptr };
};

}   // namespace cooperation_workspace

#endif   // BUTTONBOXWIDGET_H
