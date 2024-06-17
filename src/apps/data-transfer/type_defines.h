#ifndef TYPE_DEFINES_H
#define TYPE_DEFINES_H

#ifdef WIN32
#    include <QApplication>
typedef QApplication CrossApplication;
#else
#    include <DApplication>
typedef DTK_WIDGET_NAMESPACE::DApplication CrossApplication;
#endif

#endif   // TYPE_DEFINES_H
