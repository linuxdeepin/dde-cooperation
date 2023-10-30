#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#    include <QMainWindow>
typedef QMainWindow CooperationMainWindow;
#else
#    include <DMainWindow>
#    include <DAbstractDialog>
#    include <DSwitchButton>
#    include <DSuggestButton>
#    include <DSearchEdit>
typedef DTK_WIDGET_NAMESPACE::DMainWindow CooperationMainWindow;
typedef DTK_WIDGET_NAMESPACE::DAbstractDialog CooperationDialog;
typedef DTK_WIDGET_NAMESPACE::DSwitchButton CooperationSwitchButton;
typedef DTK_WIDGET_NAMESPACE::DSuggestButton CooperationSuggestButton;
typedef DTK_WIDGET_NAMESPACE::DSearchEdit CooperationSearchEdit;
#endif

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

namespace AppSettings {
inline const char kGenericGroup[] { "GenericAttribute" };
inline const char kDeviceNameKey[] { "DeviceName" };
inline const char kDiscoveryModeKey[] { "DiscoveryMode" };
inline const char kPeripheralShareKey[] { "PeripheralShare" };
inline const char kLinkDirectionKey[] { "LinkDirection" };
inline const char kTransferModeKey[] { "TransferMode" };
inline const char kStoragePathKey[] { "StoragePath" };
inline const char kClipboardShareKey[] { "ClipboardShare" };
}

// Setting menu action list
enum MenuAction {
    kSettings,
    kDownloadWindowClient
};

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

    bool operator==(const DeviceInfo &info)
    {
        return this->ipStr == info.ipStr;
    }
};

Q_DECLARE_METATYPE(DeviceInfo)

#endif   // TYPE_DEFINES_H
