// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "phonehelper.h"
#include "utils/cooperationutil.h"
#include "../cooconstrants.h"
#include "../networkutil.h"
#include <functional>
#include <QString>
#include <QMetaType>
#include <QVariantMap>

#include <gui/mainwindow.h>
#include <gui/phone/screenmirroringwindow.h>

using namespace cooperation_core;

using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
Q_DECLARE_METATYPE(ButtonStateCallback)
Q_DECLARE_METATYPE(ClickedCallback)

inline constexpr char ConnectButtonId[] { "connect-button" };
inline constexpr char DisconnectButtonId[] { "disconnect-button" };

#ifdef linux
inline constexpr char Kconnect[] { "connect" };
inline constexpr char Kdisconnect[] { "disconnect" };
#else
inline constexpr char Kconnect[] { ":/icons/deepin/builtin/texts/connect_18px.svg" };
inline constexpr char Kdisconnect[] { ":/icons/deepin/builtin/texts/disconnect_18px.svg" };
#endif

PhoneHelper::PhoneHelper(QObject *parent)
    : QObject(parent)
    , m_viewSize(0, 0)
{
}

PhoneHelper::~PhoneHelper()
{
}

PhoneHelper *PhoneHelper::instance()
{
    static PhoneHelper ins;
    return &ins;
}

void PhoneHelper::registConnectBtn(MainWindow *window)
{
    ClickedCallback clickedCb = PhoneHelper::buttonClicked;
    ButtonStateCallback visibleCb = PhoneHelper::buttonVisible;

    QVariantMap DisconnectInfo { { "id", DisconnectButtonId },
                                 { "description", tr("Disconnect") },
                                 { "icon-name", Kdisconnect },
                                 { "location", 1 },
                                 { "button-style", 0 },
                                 { "clicked-callback", QVariant::fromValue(clickedCb) },
                                 { "visible-callback", QVariant::fromValue(visibleCb) } };

    window->addMobileOperation(DisconnectInfo);
    generateQRCode(CooperationUtil::localIPAddress(), QString::number(COO_SESSION_PORT), COO_HARD_PIN);
}

void PhoneHelper::onConnect(const DeviceInfoPointer info, int w, int h)
{
    m_viewSize.setWidth(w);
    m_viewSize.setHeight(h);

    m_mobileInfo = info;
    emit addMobileInfo(info);
}

void PhoneHelper::onScreenMirroring()
{
    //todo
    if (!m_mobileInfo)
        return;
    QString mes = QString(tr("“%1”apply to initiate screen casting")).arg(m_mobileInfo.data()->deviceName());
    QStringList actions;
    actions.append(tr("cancel"));
    actions.append(tr("comfirm"));

    int res = notifyMessage(mes, actions);
    if (res != 1)
        return;

    m_screenwindow = new ScreenMirroringWindow(m_mobileInfo.data()->deviceName());
    m_screenwindow->initSizebyView(m_viewSize);
    m_screenwindow->show();

    m_screenwindow->connectVncServer(m_mobileInfo.data()->ipAddress(), 5900, "");
}

void PhoneHelper::onScreenMirroringStop()
{
    //todo
    resetScreenMirroringWindow();
}

void PhoneHelper::onScreenMirroringResize(int w, int h)
{
    if (!m_screenwindow)
        return;
    m_screenwindow->resize(w, h);
}

void PhoneHelper::onDisconnect(const DeviceInfoPointer info)
{
    if (!info)
        m_mobileInfo.reset();

    resetScreenMirroringWindow();

    emit disconnectMobile();

    if (m_mobileInfo && info && m_mobileInfo->ipAddress() == info->ipAddress()) {
        QString mes = QString(tr("“%1”connection disconnected!")).arg(m_mobileInfo.data()->deviceName());
        notifyMessage(mes, QStringList());
    }
}

int PhoneHelper::notifyMessage(const QString &message, QStringList actions)
{
    CooperationDialog dlg;
    dlg.setIcon(QIcon::fromTheme("dde-cooperation"));
    dlg.setMessage(message);

    if (actions.isEmpty())
        actions.append(tr("comfirm"));

    dlg.addButton(actions.first(), false, CooperationDialog::ButtonNormal);

    if (actions.size() > 1)
        dlg.addButton(actions[1], true, CooperationDialog::ButtonRecommend);
    int code = dlg.exec();
    return code;
}

void PhoneHelper::generateQRCode(const QString &ip, const QString &port, const QString &pin)
{
    QString combined = QString("host=%1&port=%2&pin=%3").arg(ip).arg(port).arg(pin);

    QByteArray byteArray = combined.toUtf8();
    QByteArray base64 = byteArray.toBase64();

    emit setQRCode(base64);
}

void PhoneHelper::resetScreenMirroringWindow()
{
    if (!m_screenwindow)
        return;
    delete m_screenwindow;
    m_screenwindow = nullptr;
}

void PhoneHelper::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    if (id == DisconnectButtonId) {
        QString mes = QString(tr("Are you sure to disconnect and collaborate with '%1'?")).arg(info.data()->deviceName());
        QStringList actions;
        actions.append(tr("cancel"));
        actions.append(tr("disconnect"));

        int res = PhoneHelper::instance()->notifyMessage(mes, actions);
        if (res != 1)
            return;

        NetworkUtil::instance()->disconnectRemote(info->ipAddress());
        PhoneHelper::instance()->onDisconnect(nullptr);
        return;
    }
}

bool PhoneHelper::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    if (id == ConnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connectable)
        return true;

    if (id == DisconnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connected)
        return true;

    return false;
}
