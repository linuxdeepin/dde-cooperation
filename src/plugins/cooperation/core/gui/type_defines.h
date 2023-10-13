#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#include <QMainWindow>
typedef QMainWindow CooperationMainWindow;
#else
#include <DMainWindow>
#include <DAbstractDialog>
#include <DSwitchButton>
typedef DTK_WIDGET_NAMESPACE::DMainWindow CooperationMainWindow;
typedef DTK_WIDGET_NAMESPACE::DAbstractDialog CooperationDialog;
typedef DTK_WIDGET_NAMESPACE::DSwitchButton CooperationSwitchButton;
#endif

// Setting menu action list
enum MenuAction {
    kSettings,
    kDownloadWindowClient
};

#endif   // TYPE_DEFINES_H
