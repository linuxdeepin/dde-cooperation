#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#include <QMainWindow>
typedef QMainWindow CrossMainWindow;
#else
#include <DMainWindow>
typedef DTK_WIDGET_NAMESPACE::DMainWindow CrossMainWindow ;
#endif

#endif // TYPE_DEFINES_H
