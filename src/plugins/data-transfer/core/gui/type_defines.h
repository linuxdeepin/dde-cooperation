#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#    include <QMainWindow>
typedef QMainWindow CrossMainWindow;
#else
#    include <DMainWindow>
typedef DTK_WIDGET_NAMESPACE::DMainWindow CrossMainWindow;
#endif

#ifdef WIN32
enum PageName {
    startwidget = 0,
    choosewidget = 1,
    promptwidget = 2,
    readywidget = 3,
    selectmainwidget = 4,
    transferringwidget = 5,
    successtranswidget = 6,
    filewselectidget = 7,
    configselectwidget = 8,
    appselectwidget = 9,
    errorwidget = 10,
    createbackupfilewidget = 11,
    networkdisconnectionwidget = 12,
    zipfileprocesswidget = 13,
    zipfileprocessresultwidget = 14
};
#else
enum PageName {
    startwidget = 0,
    licensewidget,
    choosewidget,
    networkdisconnectwidget,
    promptwidget,
    connectwidget,
    uploadwidget,
    waitgwidget,
    errorwidget,
    transferringwidget,
    successtranswidget,
    resultwidget
};
#endif

#endif   // TYPE_DEFINES_H
