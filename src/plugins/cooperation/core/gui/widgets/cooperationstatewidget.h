// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONSTATEWIDGET_H
#define COOPERATIONSTATEWIDGET_H

#include <QWidget>
#include "backgroundwidget.h"
#include "global_defines.h"

namespace cooperation_core {

class LookingForDeviceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LookingForDeviceWidget(QWidget *parent = nullptr);

    void seAnimationtEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();

    CooperationLabel *iconLabel { nullptr };
    QTimer *animationTimer { nullptr };
    int angle { 0 };
    bool isEnabled { false };
};

class NoNetworkWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoNetworkWidget(QWidget *parent = nullptr);

private:
    void initUI();
};

class NoResultTipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoResultTipWidget(QWidget *parent = nullptr, bool usetipMode = false);

    void onLinkActivated(const QString &link);
    void setTitleVisible(bool visible);
private:
    void initUI();
    CooperationLabel *titleLabel { nullptr };
    bool useTipMode = false;
};

class NoResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NoResultWidget(QWidget *parent = nullptr);

private:
    void initUI();
};

class BottomLabel : public QWidget
{
    Q_OBJECT
public:
    explicit BottomLabel(QWidget *parent = nullptr);

    void showDialog() const;

protected:
    void paintEvent(QPaintEvent *) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void updateSizeMode();

private:
    void initUI();

private:
    CooperationAbstractDialog *dialog { nullptr };
    QLabel *tipLabel { nullptr };
    QLabel *ipLabel { nullptr };
    QTimer *timer { nullptr };
};

class FirstTipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FirstTipWidget(QWidget *parent = nullptr);

    void setVisible(bool visible) override;
    void themeTypeChanged();

private:
    void initUI();
    QPushButton *tipBtn { nullptr };
    QLabel *firstTip { nullptr };
};

}   // namespace cooperation_core

#endif   // COOPERATIONSTATEWIDGET_H
