// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MACHINE_MACHINE_H
#define MACHINE_MACHINE_H

#include <filesystem>

#include <QVector>
#include <QString>
#include <QObject>
#include <QtDBus>

#include "common.h"

#include "protocol/message.pb.h"

class QTcpSocket;

class Manager;
class MachineDBusAdaptor;
class ClipboardBase;
class Request;
class InputEmitterWrapper;
class FuseServer;
class FuseClient;
class ReceiveTransfer;
class SendTransfer;

class Machine : public QObject, public std::enable_shared_from_this<Machine> {
    friend class Manager;
    friend class MachineDBusAdaptor;

    Q_OBJECT

public:
    Machine(Manager *manager,
            ClipboardBase *clipboard,
            QDBusConnection bus,
            uint32_t id,
            const std::filesystem::path &dataDir,
            const std::string &ip,
            uint16_t port,
            const DeviceInfo &sp);
    virtual ~Machine();

    const QString &path() const;
    const std::string &ip() const { return m_ip; };
    const std::string &uuid() const { return m_uuid; };

    void updateMachineInfo(const std::string &ip, uint16_t port, const DeviceInfo &devInfo);

    void receivedPing();
    void onPair(QTcpSocket *socket);
    void onInputGrabberEvent(uint8_t deviceType, unsigned int type, unsigned int code, int value);
    void onClipboardTargetsChanged(const std::vector<std::string> &targets);

    void flowTo(uint16_t direction, uint16_t x, uint16_t y) noexcept;
    void readTarget(const std::string &target);

    bool isPcMachine() const;
    bool isLinux() const;
    bool isAndroid() const;

    bool connected() const { return !!m_conn; }
    Manager *manager() const { return m_manager; }

protected:
    void sendServiceStatusNotification();

protected:
    QDBusConnection m_bus;
    QString m_dbusPath;

private:
    Manager *m_manager;
    MachineDBusAdaptor *m_dbusAdaptor;
    ClipboardBase *m_clipboard;

    const std::filesystem::path m_dataDir;
    const std::filesystem::path m_mountpoint;

    uint16_t m_port;
    std::string m_uuid;
    std::string m_name;
    bool m_connected;
    DeviceOS m_os;
    Compositor m_compositor;
    bool m_deviceSharing;
    uint16_t m_direction;
    bool m_sharedClipboard = false;

    QTimer *m_pingTimer;
    QTimer *m_offlineTimer;
    QTimer *m_pairTimeoutTimer;

    std::unordered_map<InputDeviceType, std::unique_ptr<InputEmitterWrapper>> m_inputEmitters;

    std::unique_ptr<FuseServer> m_fuseServer;
    std::unique_ptr<FuseClient> m_fuseClient;

    std::set<ReceiveTransfer *> m_receiveTransfers;
    uint32_t m_currentSendTransferId;
    std::unordered_map<uint32_t, SendTransfer *> m_sendTransfers;

    bool m_mounted;

    void ping();
    void onOffline();

    void initConnection();
    void initPairRequestTimer();

    void handleDisconnectedAux();
    void dispatcher() noexcept;
    void handlePairResponseAux(const PairResponse &resp);
    void handleServiceOnOffMsg(const ServiceOnOffNotification &notification);
    void handleDeviceSharingStartRequest();
    void handleDeviceSharingStartResponse(const DeviceSharingStartResponse &resp);
    void handleDeviceSharingStopRequest();
    void handleInputEventRequest(const InputEventRequest &req);
    void handleFlowDirectionNtf(const FlowDirectionNtf &ntf);
    void handleFlowRequest(const FlowRequest &req);
    void handleFsRequest(const FsRequest &req);
    void handleFsResponse(const FsResponse &resp);
    void handleFsSendFileRequest(const FsSendFileRequest &req);
    void handleFsSendFileResult(const FsSendFileResult &resp);
    void handleTransferRequest(const TransferRequest &req);
    void handleTransferResponse(const TransferResponse &resp);
    void handleStopTransferRequest(const StopTransferRequest &req);
    void handleStopTransferResponse(const StopTransferResponse &resp);
    void handleClipboardNotify(const ClipboardNotify &notify);
    void handleClipboardGetContentRequest(const ClipboardGetContentRequest &req);
    void handleClipboardGetContentResponse(const ClipboardGetContentResponse &resp);

    void stopDeviceSharingAux();
    void receivedUserConfirm(bool accepted);
    void receivedUserOperated(bool tryAgain);
    void sendFlowDirectionNtf();
    void sendReceivedFilesSystemNtf(const QString &body);
    int getPairTimeoutInterval();
    void sendPairRequest();

protected:
    QTcpSocket *m_conn;

    std::string m_ip;

    void connect();
    void disconnect();
    void requestDeviceSharing();
    void stopDeviceSharing();
    void setFlowDirection(FlowDirection direction);
    void transferSendFiles(const QStringList &filePaths);
    void sendMessage(const Message &msg);

    virtual void handleConnected() = 0;
    virtual void handleDisconnected(){};
    virtual void handleCastRequest(const CastRequest &req) = 0;

    virtual void sendFiles(const QStringList &filePaths) = 0;
};

#endif // !MACHINE_MACHINE_H
