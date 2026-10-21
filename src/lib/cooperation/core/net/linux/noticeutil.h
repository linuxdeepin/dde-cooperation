// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NOTICEUTIL_H
#define NOTICEUTIL_H

#include <QObject>
#include <QTimer>

class QDBusInterface;
namespace cooperation_core {

class NoticeUtil : public QObject
{
    Q_OBJECT

public:
    explicit NoticeUtil(QObject *parent = nullptr);
    ~NoticeUtil();

    // replaceExisting must only be set for updates of a notification that is
    // still on screen (transfer progress). A new notification always has to
    // start with replaces_id 0, otherwise some notification daemons keep
    // updating an already dismissed bubble and never show the new one.
    void notifyMessage(const QString &title, const QString &body, const QStringList &actions,
                       QVariantMap hitMap, int expireTimeout, bool replaceExisting = false);
    void closeNotification();
    void resetNotifyId();

protected:
    // Seams for tests, so the id lifecycle can be verified without a running
    // notification daemon.
    virtual uint sendNotify(const QString &title, const QString &body, const QStringList &actions,
                            const QVariantMap &hints, int expireTimeout, uint replacesId);
    virtual void sendClose(uint notificationId);

Q_SIGNALS:
    void ActionInvoked(const QString &action);
    void onConfirmTimeout();

private Q_SLOTS:
    void onActionTriggered(uint replacesId, const QString &action);

private:
    void initNotifyConnect();

private:
    QTimer confirmTimer;

    QDBusInterface *notifyIfc { nullptr };
    uint recvNotifyId { 0 };
};

}   // namespace cooperation_core

#endif   // NOTICEUTIL_H
