// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BACKGROUNDWIDGET_H
#define BACKGROUNDWIDGET_H

#include <DFrame>
#include <DPalette>

#include <QFrame>

namespace cooperation_workspace {

class BackgroundWidget : public QFrame
{
    Q_OBJECT

public:
    enum ColorType {
        kNoType = DTK_GUI_NAMESPACE::DPalette::NoType,
        kItemBackground = DTK_GUI_NAMESPACE::DPalette::ItemBackground   //列表项的背景色
    };

    explicit BackgroundWidget(QWidget *parent = nullptr);

    void setBackground(int radius, ColorType colorType);

protected:
    void resizeEvent(QResizeEvent *event) override;

protected:
    DTK_WIDGET_NAMESPACE::DFrame *bgGroup { nullptr };
};

}   // namespace cooperation_workspace

#endif   // BACKGROUNDWIDGET_H
