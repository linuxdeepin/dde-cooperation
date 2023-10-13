#ifndef GLOBAL_DEFINES_H
#define GLOBAL_DEFINES_H

#ifdef WIN32
#else
#include <DDialog>
#include <DSpinner>
typedef DTK_WIDGET_NAMESPACE::DDialog CooperationDialog;
typedef DTK_WIDGET_NAMESPACE::DSpinner CooperationSpinner;
#endif

inline constexpr char kHistoryId[] { "history-id" };
inline constexpr char kTransferId[] { "transfer-id" };

#endif   // GLOBAL_DEFINES_H
