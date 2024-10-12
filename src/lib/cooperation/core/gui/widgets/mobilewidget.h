// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MOBILEWIDGET_H
#define MOBILEWIDGET_H

#include "global_defines.h"
#include "discover/deviceinfo.h"
#include <QWidget>

class QStackedLayout;
namespace cooperation_core {
class NoNetworkWidget;
class DeviceListWidget;
class BottomLabel;
class ConnectWidget;

class MobileWidget : public QWidget
{
    Q_OBJECT
public:
    enum PageName {
        kLookignForDeviceWidget = 0,
        kNoNetworkWidget,
        kNoResultWidget,
        kDeviceListWidget,

        kUnknownPage = 99,
    };

    explicit MobileWidget(QWidget *parent = nullptr);
    void initUI();
    void switchWidget(PageName page);

private:
    QStackedLayout *stackedLayout { nullptr };
    CooperationSearchEdit *searchEdit { nullptr };
    NoNetworkWidget *nnWidget { nullptr };
    DeviceListWidget *dlWidget { nullptr };
    ConnectWidget *conWidget { nullptr };
};

class ConnectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConnectWidget(QWidget *parent = nullptr);

    void initUI();
    QPixmap generateQRCode(const QString &text, int scale = 10);
    void setInfo(const QString &info);

private:
    QLabel *qrCode { nullptr };
};

}   // namespace cooperation_core

#endif   // MOBILEWIDGET_H
