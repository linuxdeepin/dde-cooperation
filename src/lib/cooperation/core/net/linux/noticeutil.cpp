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

    emit ActionInvoked(action);
}

void NoticeUtil::notifyMessage(const QString &title, const QString &body, const QStringList &actions, QVariantMap hitMap, int expireTimeout)
{
    // If hitMap is not empty, use previous recvNotifyId for progress updates
    uint notifyId = hitMap.isEmpty() ? 0 : recvNotifyId;
    
    QDBusReply<uint> reply = notifyIfc->call(QString("Notify"), QString("dde-cooperation"), notifyId,
                                             QString("dde-cooperation"), title, body,
                                             actions, hitMap, expireTimeout);

    recvNotifyId = reply.value();
}

void NoticeUtil::closeNotification()
{
    notifyIfc->call("CloseNotification", recvNotifyId);
}

void NoticeUtil::resetNotifyId()
{
    DLOG << "Resetting notification ID";
    recvNotifyId = 0;
    DLOG << "Notification ID reset complete";
}
