#ifndef GLOBAL_DEFINES_H
#define GLOBAL_DEFINES_H

#include <QString>
#include <QObject>

namespace OperationKey {
inline constexpr char kID[] { "id" };
inline constexpr char kIconName[] { "icon-name" };
inline constexpr char kButtonStyle[] { "button-style" };
inline constexpr char kLocation[] { "location" };
inline constexpr char kDescription[] { "description" };
inline constexpr char kClickedCallback[] { "clicked-callback" };
inline constexpr char kVisibleCallback[] { "visible-callback" };
inline constexpr char kClickableCallback[] { "clickable-callback" };
}

enum ConnectState {
    kConnected,
    kConnectable,
    kOffline
};

struct DeviceInfo
{
    QString deviceName;
    QString ipStr;
    ConnectState state;
};

Q_DECLARE_METATYPE(DeviceInfo)

#endif   // GLOBAL_DEFINES_H
