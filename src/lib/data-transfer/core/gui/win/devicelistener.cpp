#include "devicelistener.h"
#include "common/log.h"

#include <Windows.h>
#include <QStorageInfo>
#include <dbt.h>
#include <QDebug>

DeviceListener::DeviceListener(QWidget *parent) : QWidget(parent)
{
    DLOG << "Initializing device listener";

    setFixedSize(0, 0);
    updateDevice();
}

DeviceListener::~DeviceListener()
{
    DLOG << "Destroying device listener";
}

DeviceListener *DeviceListener::instance()
{
    DLOG << "Getting device listener instance";

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
            DLOG << "Device connected";
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            DLOG << "Device disconnected";
            break;
        }
    }
    updateDevice();
    return QWidget::nativeEvent(eventType, message, result);
}
