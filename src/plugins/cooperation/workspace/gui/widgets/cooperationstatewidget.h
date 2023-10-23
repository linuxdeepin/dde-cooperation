// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONSTATEWIDGET_H
#define COOPERATIONSTATEWIDGET_H

#include <QWidget>

namespace cooperation_workspace {

class LookingForDeviceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LookingForDeviceWidget(QWidget *parent = nullptr);

private:
    void initUI();
};

class NoNetworkWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoNetworkWidget(QWidget *parent = nullptr);

private:
    void initUI();
};

class NoResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoResultWidget(QWidget *parent = nullptr);

public Q_SLOTS:
    void onLinkActivated(const QString &link);

private:
    void initUI();
};

}   // namespace cooperation_workspace

#endif   // COOPERATIONSTATEWIDGET_H
