#include "noticeutil.h"

#include <QDBusInterface>
#include <QDBusReply>

inline constexpr char NotifyServerName[] { "org.freedesktop.Notifications" };
inline constexpr char NotifyServerPath[] { "/org/freedesktop/Notifications" };
inline constexpr char NotifyServerIfce[] { "org.freedesktop.Notifications" };

using namespace cooperation_core;

NoticeUtil::NoticeUtil(QObject *parent)
    : QObject(parent)
{
}

NoticeUtil::~NoticeUtil()
{
}

void NoticeUtil::initNotifyConnect()
{
    transTimer.setInterval(10 * 1000);
    transTimer.setSingleShot(true);

    connect(&transTimer, &QTimer::timeout, this, &NoticeUtil::onConfirmTimeout);

    notifyIfc = new QDBusInterface(NotifyServerName,
                                   NotifyServerPath,
                                   NotifyServerIfce,
                                   QDBusConnection::sessionBus(), this);
    QDBusConnection::sessionBus().connect(NotifyServerName, NotifyServerPath, NotifyServerIfce, "ActionInvoked",
                                          this, SLOT(onActionTriggered(uint, const QString &)));
}

void NoticeUtil::onActionTriggered(uint replacesId, const QString &action)
{
    if (replacesId != recvNotifyId)
        return;

    emit ActionInvoked(action);
}

void NoticeUtil::notifyMessage(uint replacesId, const QString &title, const QString &body, const QStringList &actions, QVariantMap hitMap, int expireTimeout)
{
    recvNotifyId = 0;
    QDBusReply<uint> reply = notifyIfc->call("Notify", "dde-cooperation", replacesId,
                                             "dde-cooperation", title, body,
                                             actions, hitMap, expireTimeout);

    recvNotifyId = reply.isValid() ? reply.value() : replacesId;
}
