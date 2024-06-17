#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

namespace AppSettings {
inline constexpr char GenericGroup[] { "GenericAttribute" };
inline constexpr char DeviceNameKey[] { "DeviceName" };
inline constexpr char DiscoveryModeKey[] { "DiscoveryMode" };
inline constexpr char PeripheralShareKey[] { "PeripheralShare" };
inline constexpr char LinkDirectionKey[] { "LinkDirection" };
inline constexpr char TransferModeKey[] { "TransferMode" };
inline constexpr char StoragePathKey[] { "StoragePath" };
inline constexpr char ClipboardShareKey[] { "ClipboardShare" };
inline constexpr char CooperationEnabled[] { "CooperationEnabled" };

inline constexpr char CacheGroup[] { "Cache" };
inline constexpr char TransHistoryKey[] { "TransHistory" };
}

namespace DConfigKey {
inline constexpr char DiscoveryModeKey[] { "cooperation.discovery.mode" };
inline constexpr char TransferModeKey[] { "cooperation.transfer.mode" };
}

inline const char MainAppName[] { "dde-cooperation" };
inline const char PluginName[] { "daemon-cooperation" };

#endif   // TYPE_DEFINES_H
