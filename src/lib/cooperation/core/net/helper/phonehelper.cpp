// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "phonehelper.h"
#include "utils/cooperationutil.h"
#include "../cooconstrants.h"
#include "../networkutil.h"
#include "common/log.h"

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

inline constexpr char KprotocolVer[] { "1.0.0" };

PhoneHelper::PhoneHelper(QObject *parent)
    : QObject(parent), m_viewSize(0, 0)
{
    DLOG << "PhoneHelper constructor";
}

PhoneHelper::~PhoneHelper()
{
    DLOG << "PhoneHelper destructor";
}

PhoneHelper *PhoneHelper::instance()
{
    DLOG << "Getting PhoneHelper instance";
    static PhoneHelper ins;
    return &ins;
}

void PhoneHelper::registConnectBtn(MainWindow *window)
{
    DLOG << "Registering mobile connection buttons";
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
    DLOG << "Generating QR code for mobile connection";
    generateQRCode(CooperationUtil::localIPAddress(), QString::number(COO_SESSION_PORT), COO_HARD_PIN);
    DLOG << "Mobile connection buttons registered";
}

void PhoneHelper::onConnect(const DeviceInfoPointer info, int w, int h)
{
    m_viewSize.setWidth(w);
    m_viewSize.setHeight(h);

    m_mobileInfo = info;
    emit addMobileInfo(info);
    DLOG << "Emitted addMobileInfo signal";
}

void PhoneHelper::onScreenMirroring()
{
    DLOG << "onScreenMirroring called";
    //todo
    if (!m_mobileInfo) {
        DLOG << "No mobile info, returning";
        return;
    }
    QString mes = QString(tr("“%1”apply to initiate screen casting")).arg(m_mobileInfo.data()->deviceName());
    QStringList actions;
    actions.append(tr("cancel"));
    actions.append(tr("comfirm"));

    DLOG << "Showing screen mirroring confirmation dialog";
    int res = notifyMessage(mes, actions);
    if (res != 1) {
        DLOG << "Screen mirroring not confirmed, returning";
        return;
    }

    DLOG << "Creating screen mirroring window";
    m_screenwindow = new ScreenMirroringWindow(m_mobileInfo.data()->deviceName());
    m_screenwindow->initSizebyView(m_viewSize);
    m_screenwindow->show();

    m_screenwindow->connectVncServer(m_mobileInfo.data()->ipAddress(), 5900, "");
}

void PhoneHelper::onScreenMirroringStop()
{
    resetScreenMirroringWindow();
    DLOG << "Screen mirroring stopped";
}

void PhoneHelper::onScreenMirroringResize(int w, int h)
{
    DLOG << "Screen mirroring resize requested:" << w << "x" << h;
    if (!m_screenwindow) {
        DLOG << "No screen window, returning";
        return;
    }
    m_screenwindow->resize(w, h);
}

void PhoneHelper::onDisconnect(const DeviceInfoPointer info)
{
    if (!info) {
        DLOG << "Resetting mobile info";
        m_mobileInfo.reset();
    } else {
        DLOG << "Disconnecting specific device:" << info->deviceName().toStdString();
    }

    resetScreenMirroringWindow();

    DLOG << "Emitting disconnectMobile signal";
    emit disconnectMobile();

    if (m_mobileInfo && info && m_mobileInfo->ipAddress() == info->ipAddress()) {
        QString mes = QString(tr("“%1”connection disconnected!")).arg(m_mobileInfo.data()->deviceName());
        DLOG << "Showing disconnection notification";
        notifyMessage(mes, QStringList());
    }
}

int PhoneHelper::notifyMessage(const QString &message, QStringList actions)
{
    if (isInNotify)
        return -1;

    isInNotify = true;

    CooperationDialog dlg(qApp->activeWindow());
    dlg.setIcon(QIcon::fromTheme("dde-cooperation"));
    dlg.setMessage(message);

    if (actions.isEmpty()) {
        DLOG << "Actions list is empty, adding default 'comfirm' action";
        actions.append(tr("comfirm"));
    }

    dlg.addButton(actions.first(), false, CooperationDialog::ButtonNormal);

    if (actions.size() > 1) {
        DLOG << "More than one action, adding second button";
        dlg.addButton(actions[1], true, CooperationDialog::ButtonRecommend);
    }

    if (qApp->activeWindow()) {
        QWidget *activeWindow = qApp->activeWindow();
        QSize activeSize = activeWindow->size();
        QSize dlgSize = dlg.size();

        // 计算新位置，使对话框位于活动窗口的中心
        QPoint newPos = activeWindow->pos() + QPoint((activeSize.width() - dlgSize.width()) / 2, (activeSize.height() - dlgSize.height()) / 2);
        dlg.move(newPos);
    }

    int code = dlg.exec();

    isInNotify = false;
    return code;
}

void PhoneHelper::generateQRCode(const QString &ip, const QString &port, const QString &pin)
{
    QString combined = QString("host=%1&port=%2&pin=%3&pv=%4").arg(ip).arg(port).arg(pin).arg(KprotocolVer);

    QByteArray byteArray = combined.toUtf8();
    QByteArray base64 = byteArray.toBase64();
    QString qrContent = QString("%1?mark=%2").arg(KdownloadUrl).arg(QString::fromUtf8(base64));

    emit setQRCode(qrContent);
    DLOG << "QR code signal emitted";
}

void PhoneHelper::resetScreenMirroringWindow()
{
    if (!m_screenwindow) {
        DLOG << "No screen mirroring window to reset";
        return;
    }

    DLOG << "Resetting screen mirroring window";
    m_screenwindow->deleteLater();
    m_screenwindow = nullptr;
    DLOG << "Screen mirroring window reset complete";
}

void PhoneHelper::buttonClicked(const QString &id, const DeviceInfoPointer info)
{
    if (id == DisconnectButtonId) {
        DLOG << "Disconnect requested for device:"
                          << info->deviceName().toStdString();
        QString mes = QString(tr("Are you sure to disconnect and collaborate with '%1'?")).arg(info.data()->deviceName());
        QStringList actions;
        actions.append(tr("cancel"));
        actions.append(tr("disconnect"));

        int res = PhoneHelper::instance()->notifyMessage(mes, actions);
        if (res != 1) {
            DLOG << "Disconnect canceled by user";
            return;
        }

        NetworkUtil::instance()->disconnectRemote(info->ipAddress());
        DLOG << "Disconnecting from device:" << info->ipAddress().toStdString();
        PhoneHelper::instance()->onDisconnect(nullptr);
        return;
    }
}

bool PhoneHelper::buttonVisible(const QString &id, const DeviceInfoPointer info)
{
    if (id == ConnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connectable) {
        DLOG << "Button ID is ConnectButtonId and device is Connectable, returning true";
        return true;
    }

    if (id == DisconnectButtonId && info->connectStatus() == DeviceInfo::ConnectStatus::Connected) {
        DLOG << "Button ID is DisconnectButtonId and device is Connected, returning true";
        return true;
    }

    DLOG << "Button not visible, returning false";
    return false;
}
