#ifndef DEVICELISTENER_H
#define DEVICELISTENER_H

#include <QWidget>

class QVariant;
class QStorageInfo;
class DeviceListener : public QWidget
{
    Q_OBJECT
public:
    DeviceListener(QWidget *parent = nullptr);
    static DeviceListener *instance();
    ~DeviceListener();
signals:
    void updateDevice();
protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    bool enroll{ false };
    QList<QStorageInfo> deviceList;
};

#endif // DEVICELISTENER_H
