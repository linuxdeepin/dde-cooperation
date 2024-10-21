// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCREENMIRRIORINGWINDOW_H
#define SCREENMIRRIORINGWINDOW_H

#include "global_defines.h"
#include <QWindow>
#include <QPainter>
#include <QPixmap>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScreen>
#include <DMainWindow>
#include <DTitlebar>
#include <DIconButton>
#include <DButtonBox>

#include <QVBoxLayout>
#include <QApplication>
#include <QStackedLayout>

namespace cooperation_core {

class ScreenMirroringWindow : public CooperationMainWindow
{
    Q_OBJECT

public:
    enum Operation {
        BACK = 0,
        HOME,
        MULTI_TASK
    };

    explicit ScreenMirroringWindow(const QString &device, QWidget *parent = nullptr);

    void initTitleBar(const QString &device);
    void initBottom();
    void initWorkWidget();

Q_SIGNALS:
    void ButtonClicked(int opertion);

private:
    QStackedLayout *stackedLayout { nullptr };
    QWidget *bottomWidget { nullptr };
    QWidget *workWidget { nullptr };
};

class LockScreenWidget : public QWidget
{
public:
    LockScreenWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

}   // namespace cooperation_core

#endif   // SCREENMIRRIORINGWINDOW_H
