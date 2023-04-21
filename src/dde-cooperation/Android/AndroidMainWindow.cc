// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AndroidMainWindow.h"

#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QDebug>
#include <QMainWindow>

#include <QQuickWidget>
#include <QQuickView>
#include <QQmlEngine>
#include <QQmlContext>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QUrlQuery>

#include "QtScrcpyCore.h"
#include "QrCodeProxy.h"
#include "DeviceProxy.h"
#include "VideoFrameProvider.h"

DTK_USE_NAMESPACE

using namespace qsc;

#define DDE_PROTO_VER 13

AndroidMainWindow::AndroidMainWindow(const QString &uuid, QObject *parent)
    : QObject(parent)
    , m_stage(0)
    , m_uuid(uuid)
    , m_adb(new AdbProcess(this))
    , m_qrCodeProxy(new QrCodeProxy(this))
    , m_deviceProxy(new DeviceProxy(this))
    , m_videoFrameProvider(new VideoFrameProvider(this)) {

    engine.rootContext()->setContextProperty("backend", this);
    engine.rootContext()->setContextProperty("qrCode", m_qrCodeProxy);
    engine.rootContext()->setContextProperty("device", m_deviceProxy);
    engine.rootContext()->setContextProperty("videoFrameProvider", m_videoFrameProvider);
    const QUrl url(QStringLiteral("qrc:///qml/MainWindow.qml"));
    engine.load(url);

    connect(m_adb, &AdbProcess::adbProcessResult, this, &AndroidMainWindow::handleAdbProcessResult);
}

AndroidMainWindow::~AndroidMainWindow() {
}

void AndroidMainWindow::showConnectDevice() {
    updateQrCode();

    setStage(STAGE_SHOW_QR_CODE);
}

void AndroidMainWindow::closeWindow() {
    deleteLater();
}

void AndroidMainWindow::sendFiles(const QList<QUrl> &urls) {
    qInfo() << "urls:" << urls;
    QStringList paths;
    paths.reserve(urls.size());

    for (const auto &url : urls) {
        if (url.isLocalFile()) {
            paths.append(url.toLocalFile());
        }
    }

    emit requestSendFiles(paths);
}

QStringList AndroidMainWindow::getIPs() {
    QStringList ips;
    for (const QHostAddress &address : QNetworkInterface::allAddresses()) {
        if (address.isLoopback()) {
            continue;
        }

        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            ips.append(address.toString());
        }
    }

    return ips;
}

void AndroidMainWindow::updateQrCode() {
    QUrlQuery query;
    query.addQueryItem("dde", QString::number(DDE_PROTO_VER));
    query.addQueryItem("host", m_uuid);
    query.addQueryItem("ip", getIPs().join(";"));
    QString qrcodeText = query.toString();

    m_qrCodeProxy->setText(qrcodeText);
}

void AndroidMainWindow::setStage(STAGE stage) {
    m_stage = stage;
    emit stageChanged(m_stage);
}

void AndroidMainWindow::handleAdbProcessResult(qsc::AdbProcess::ADB_EXEC_RESULT processResult) {
    QStringList args = m_adb->arguments();
    switch (processResult) {
    case AdbProcess::AER_SUCCESS_START:
        break;
    case AdbProcess::AER_ERROR_START:
        break;
    case AdbProcess::AER_SUCCESS_EXEC:
        if (args.contains("devices")) {
            QStringList devices = m_adb->getDevicesSerialFromStdOut();
            if (!devices.contains(m_tcpSerial)) {
                connectTCPAdb();
            } else {
                QTimer::singleShot(500, this, [this]() {
                    connectDevice(); // 打开投屏端口准备投屏
                    emit tcpAdbConnected();
                });
            }
        } else if (args.contains("tcpip")) {
            connectTCPAdb();
        } else if (args.contains("connect")) {
            QTimer::singleShot(500, this, [this]() {
                connectDevice(); // 打开投屏端口准备投屏
                emit tcpAdbConnected();
            });
        }
        break;
    case AdbProcess::AER_ERROR_EXEC:
        break;
    case AdbProcess::AER_ERROR_MISSING_BINARY:
        break;
    };
}

void AndroidMainWindow::listDevices() {
    m_adb->execute("", QStringList{"devices", "-l"});
}

void AndroidMainWindow::setWirelessDbgAddress(const QString &ip, uint16_t port) {
    m_ip = ip;
    m_tcpPort = port;
    m_tcpSerial = QString("%1:%2").arg(m_ip).arg(m_tcpPort);
}

void AndroidMainWindow::openTCPAdb() {
    m_adb->execute("", QStringList{"tcpip", QString::number(m_tcpPort)});
}

void AndroidMainWindow::ensureTCPAdbConnected() {
    listDevices();
}

void AndroidMainWindow::connectTCPAdb() {
    m_adb->execute("", QStringList{"connect", m_tcpSerial});
}

void AndroidMainWindow::connectDevice() {
    DeviceParams params;
    params.serial = m_tcpSerial;
    m_device = IDevice::create(params, this);
    m_deviceProxy->setDevice(m_device);

    connect(m_device, &IDevice::deviceConnected, this, &AndroidMainWindow::deviceConnected);
    connect(m_device, &IDevice::deviceDisconnected, this, &AndroidMainWindow::deviceDisconnected);

    m_device->connectDevice();
}

void AndroidMainWindow::deviceConnected(bool success,
                                        [[maybe_unused]] const QString &serial,
                                        [[maybe_unused]] const QString &deviceName,
                                        [[maybe_unused]] const QSize &size) {
    qDebug() << "deviceConnected:" << success;
    if (!success) {
        return;
    }

    m_stage = STAGE_ALREADY;
    emit stageChanged(m_stage);
    m_device->registerDeviceObserver(m_videoFrameProvider);

    // setWindowTitle(deviceName + " - " + serial);
}

void AndroidMainWindow::deviceDisconnected([[maybe_unused]] const QString &serial) {
    deleteLater();
}
