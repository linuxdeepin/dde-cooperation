#include "devicelistener.h"

#include <Windows.h>
#include <QStorageInfo>
#include <dbt.h>
#include <QDebug>

DeviceListener::DeviceListener(QWidget *parent) : QWidget(parent)
{
    setFixedSize(0, 0);
    updateDevice();
    qInfo() << "DeviceListener init";
}

DeviceListener::~DeviceListener() { }

DeviceListener *DeviceListener::instance()
{
    static DeviceListener ins;

    if (!ins.enroll) {
        ins.show();
        ins.hide();
        ins.enroll = true;
    }
    return &ins;
}

bool DeviceListener::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG *msg = reinterpret_cast<MSG *>(message);
    if (msg->message == WM_DEVICECHANGE) {
        switch (msg->wParam) {
        case DBT_DEVICEARRIVAL:
            qInfo() << "Device added";
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            qInfo() << "Device removed";
            break;
        }
    }
    updateDevice();
    return QWidget::nativeEvent(eventType, message, result);
}
