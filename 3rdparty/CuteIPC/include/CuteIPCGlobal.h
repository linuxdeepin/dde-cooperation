#ifndef CUTEIPCGLOBAL_H
#define CUTEIPCGLOBAL_H

#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #define CUTE_IPC_ARG QMetaMethodArgument
    #define CUTE_IPC_RETURN_ARG QMetaMethodReturnArgument
#else
    #define CUTE_IPC_ARG QGenericArgument
    #define CUTE_IPC_RETURN_ARG QGenericReturnArgument
#endif

#endif // CUTEIPCGLOBAL_H 