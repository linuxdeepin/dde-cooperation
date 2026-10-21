#include "noticeutil.h"
#include "common/log.h"

#include <QDBusInterface>
#include <QDBusReply>

inline constexpr char NotifyServerName[] { "org.freedesktop.Notifications" };
inline constexpr char NotifyServerPath[] { "/org/freedesktop/Notifications" };
inline constexpr char NotifyServerIfce[] { "org.freedesktop.Notifications" };

using namespace cooperation_core;

NoticeUtil::NoticeUtil(QObject *parent)
    : QObject(parent)
{
    DLOG << "NoticeUtil constructor";
    initNotifyConnect();
    DLOG << "NoticeUtil initialized";
}

NoticeUtil::~NoticeUtil()
{
    DLOG << "NoticeUtil destructor";
}

void NoticeUtil::initNotifyConnect()
{
    DLOG << "Initializing notification connection";
    confirmTimer.setInterval(10 * 1000);
    confirmTimer.setSingleShot(true);
    DLOG << "Set confirm timer to 10 seconds";

    connect(&confirmTimer, &QTimer::timeout, this, &NoticeUtil::onConfirmTimeout);
    DLOG << "Connected confirm timer signal";

    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
}

void NoticeUtil::onActionTriggered(uint replacesId, const QString &action)
{
    DLOG << "Notification action triggered, ID:" << replacesId << "action:" << action.toStdString();
    
    if (replacesId != recvNotifyId) {
        DLOG << "Notification ID mismatch (expected:" << recvNotifyId
                          << "got:" << replacesId << "), ignoring action";
        return;
    }

    // Invoking an action dismisses the notification, so its id is no longer
    // usable for follow-up updates.
    recvNotifyId = 0;
    emit ActionInvoked(action);
}

void NoticeUtil::notifyMessage(const QString &title, const QString &body, const QStringList &actions,
                               QVariantMap hitMap, int expireTimeout, bool replaceExisting)
{
    uint notifyId = replaceExisting ? recvNotifyId : 0;
    uint shownId = sendNotify(title, body, actions, hitMap, expireTimeout, notifyId);
    if (shownId == 0) {
        // Do not keep a stale id around: the next notification must be able to
        // create a fresh bubble.
        WLOG << "Failed to show notification, replaces_id:" << notifyId;
        recvNotifyId = 0;
        return;
    }

    recvNotifyId = shownId;
}

uint NoticeUtil::sendNotify(const QString &title, const QString &body, const QStringList &actions,
                            const QVariantMap &hints, int expireTimeout, uint replacesId)
{
    QDBusReply<uint> reply = notifyIfc->call(QString("Notify"), QString("dde-cooperation"), replacesId,
                                             QString("dde-cooperation"), title, body,
                                             actions, hints, expireTimeout);
    if (!reply.isValid()) {
        WLOG << "Notification D-Bus call failed:" << reply.error().message().toStdString();
        return 0;
    }

    return reply.value();
}

void NoticeUtil::sendClose(uint notificationId)
{
    notifyIfc->call("CloseNotification", notificationId);
}

void NoticeUtil::closeNotification()
{
    if (recvNotifyId == 0)
        return;

    sendClose(recvNotifyId);
    recvNotifyId = 0;
}

void NoticeUtil::resetNotifyId()
{
    DLOG << "Resetting notification ID";
    recvNotifyId = 0;
    DLOG << "Notification ID reset complete";
}
