#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#include <QMainWindow>
typedef QMainWindow CooperationMainWindow;
#else
#include <DMainWindow>
#include <DAbstractDialog>
#include <DSwitchButton>
#include <DSuggestButton>
#include <DSearchEdit>
#include <DDialog>
#include <DSpinner>
#include <DIconButton>
#include <DFloatingButton>
typedef DTK_WIDGET_NAMESPACE::DDialog CooperationDialog;
typedef DTK_WIDGET_NAMESPACE::DSpinner CooperationSpinner;
typedef DTK_WIDGET_NAMESPACE::DMainWindow CooperationMainWindow;
typedef DTK_WIDGET_NAMESPACE::DAbstractDialog CooperationAbstractDialog;
typedef DTK_WIDGET_NAMESPACE::DSwitchButton CooperationSwitchButton;
typedef DTK_WIDGET_NAMESPACE::DSuggestButton CooperationSuggestButton;
typedef DTK_WIDGET_NAMESPACE::DSearchEdit CooperationSearchEdit;
typedef DTK_WIDGET_NAMESPACE::DIconButton CooperationIconButton;
typedef DTK_WIDGET_NAMESPACE::DFloatingButton CooperationFloatingEdit;
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
inline const char GenericGroup[] { "GenericAttribute" };
inline const char DeviceNameKey[] { "DeviceName" };
inline const char DiscoveryModeKey[] { "DiscoveryMode" };
inline const char PeripheralShareKey[] { "PeripheralShare" };
inline const char LinkDirectionKey[] { "LinkDirection" };
inline const char TransferModeKey[] { "TransferMode" };
inline const char StoragePathKey[] { "StoragePath" };
inline const char ClipboardShareKey[] { "ClipboardShare" };
}

inline const char kMainAppName[] { "dde-cooperation" };

// Setting menu action list
enum MenuAction {
    kSettings,
    kDownloadWindowClient
};

#endif   // TYPE_DEFINES_H
