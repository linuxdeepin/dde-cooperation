#ifndef GLOBAL_DEFINES_H
#define GLOBAL_DEFINES_H

#ifdef WIN32
#else
#include <DDialog>
#include <DSpinner>
typedef DTK_WIDGET_NAMESPACE::DDialog CooperationDialog;
typedef DTK_WIDGET_NAMESPACE::DSpinner CooperationSpinner;
#endif

inline constexpr char kHistoryButtonId[] { "history-id" };
inline constexpr char kTransferButtonId[] { "transfer-id" };
inline constexpr char kStoragePathKey[] { "StoragePath" };
inline constexpr char kTransferModeKey[] { "TransferMode" };

inline constexpr char kNotifyCancelAction[] { "cancel-action" };
inline constexpr char kNotifyRefuseAction[] { "refuse-action" };
inline constexpr char kNotifyAcceptAction[] { "accept-action" };
inline constexpr char kNotifyViewAction[] { "view-action" };

enum TransferStatus {
    Idle,
    Connecting,
    Transfering
};

struct TransferInfo
{
    int64_t totalSize;   // 总量
    int64_t transferSize;   // 当前传输量
    int32_t maxTimeSec;   // 耗时
};

enum TransferSettings
{
    Everyone = 0,
    OnlyConnected,
    NotAllow
};

#endif   // GLOBAL_DEFINES_H
