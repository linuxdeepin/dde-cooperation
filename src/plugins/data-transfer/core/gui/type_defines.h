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
    choosewidget ,
    promptwidget,
    readywidget,
    selectmainwidget,
    transferringwidget,
    successtranswidget,
    filewselectidget,
    configselectwidget,
    appselectwidget,
    errorwidget,
    createbackupfilewidget,
    networkdisconnectwidget,
    zipfileprocesswidget,
    zipfileprocessresultwidget
};
#else
enum PageName {
    startwidget = 0,
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
