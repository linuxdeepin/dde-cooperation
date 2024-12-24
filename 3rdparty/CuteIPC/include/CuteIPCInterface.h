#ifndef CUTEIPCINTERFACE_H
#define CUTEIPCINTERFACE_H

#include "exportglobal.h"
#include "CuteIPCGlobal.h"

#if __cplusplus >= 201402L
#  define DECL_DEPRECATED(x) [[deprecated(x)]]
#elif defined(__GNUC__)
#  define DECL_DEPRECATED(x) __attribute__((deprecated(x)))
#elif defined(_MSC_VER)
// 暂时移除或注释掉这部分代码
// #  define DECL_DEPRECATED(x) __pragma deprecated(x)
#  define DECL_DEPRECATED(x)
#else
#  define DECL_DEPRECATED(x)
#endif

// Qt
#include <QObject>
class QHostAddress;

// Local
class CuteIPCInterfacePrivate;


class EXPORT_API CuteIPCInterface : public QObject
{
  Q_OBJECT

  public:
    CuteIPCInterface(QObject* parent = 0);
    ~CuteIPCInterface();

    bool connectToServer(const QString& name);
    bool connectToServer(const QHostAddress& host, quint16 port);

    void disconnectFromServer();
    bool isConnected();
    bool remoteConnect(const char* signal, QObject* object, const char* method);
    bool remoteConnect(QObject* localObject, const char* localSignal, const char* remoteMethod);

    DECL_DEPRECATED("Replaced by remoteConnect(), which can connect to both signals and slots")
    bool remoteSlotConnect(QObject* localObject, const char* signal, const char* remoteSlot);

    bool disconnectSignal(const char* signal, QObject* object, const char* method);
    bool disconnectRemoteMethod(QObject* localObject, const char* signal, const char* remoteMethod);

    DECL_DEPRECATED("Replaced by disconnectRemoteMethod(), which can disconnect from both signals and slots")
    bool disconnectSlot(QObject* localObject, const char* signal, const char* remoteSlot);

    bool call(const QString& method, CUTE_IPC_RETURN_ARG ret,
              CUTE_IPC_ARG val0 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val1 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val2 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val3 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val4 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val5 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val6 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val7 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val8 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val9 = CUTE_IPC_ARG());

    bool call(const QString& method,
              CUTE_IPC_ARG val0 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val1 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val2 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val3 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val4 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val5 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val6 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val7 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val8 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val9 = CUTE_IPC_ARG());

    void callNoReply(const QString& method,
              CUTE_IPC_ARG val0 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val1 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val2 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val3 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val4 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val5 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val6 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val7 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val8 = CUTE_IPC_ARG(),
              CUTE_IPC_ARG val9 = CUTE_IPC_ARG());


    QString lastError() const;

  signals:
    void connected();
    void disconnected();


  protected:
    CuteIPCInterfacePrivate* const d_ptr;
    CuteIPCInterface(CuteIPCInterfacePrivate& dd, QObject* parent);

    bool callImpl(const QString& method, 
                 const QGenericReturnArgument& ret,
                 const QGenericArgument* args,
                 int argc);

  private:
    Q_DECLARE_PRIVATE(CuteIPCInterface)
    Q_PRIVATE_SLOT(d_func(),void _q_sendAsynchronousRequest(QByteArray))
    Q_PRIVATE_SLOT(d_func(),void _q_removeSignalHandlersOfObject(QObject*))
    Q_PRIVATE_SLOT(d_func(), void _q_setLastError(QString))
    Q_PRIVATE_SLOT(d_func(), void _q_invokeRemoteSignal(QString, CuteIPCMessage::Arguments))
    Q_PRIVATE_SLOT(d_func(), void _q_removeRemoteConnectionsOfObject(QObject*))
};

#endif // CUTEIPCINTERFACE_H
