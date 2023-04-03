// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Machine.h"

#include <condition_variable>

#include <fmt/core.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

#include <DDBusSender>

#include "Manager.h"
#include "MachineDBusAdaptor.h"
#include "Wrappers/InputEmitterWrapper.h"
#include "ConfirmDialog.h"
#include "Fuse/FuseServer.h"
#include "Fuse/FuseClient.h"
#include "utils/message_helper.h"
#include "ReconnectDialog.h"
#include "ReceiveTransfer.h"
#include "SendTransfer.h"

#include "protocol/message.pb.h"

#include "utils/net.h"

namespace fs = std::filesystem;

static const std::string fileSchema{"file://"};
static const std::string clipboardFileTarget{"x-special/gnome-copied-files"};
static const std::string uriListTarget{"text/uri-list"};
const static QString dConfigAppID = "org.deepin.cooperation";
const static QString dConfigName = "org.deepin.cooperation";

static const uint64_t U10s = 10 * 1000;
static const uint64_t U25s = 25 * 1000;

Machine::Machine(Manager *manager,
                 ClipboardBase *clipboard,
                 QDBusConnection bus,
                 uint32_t id,
                 const fs::path &dataDir,
                 const std::string &ip,
                 uint16_t port,
                 const DeviceInfo &sp)
    : m_bus(bus)
    , m_dbusPath(QString("/org/deepin/dde/Cooperation1/Machine/%1").arg(id))
    , m_manager(manager)
    , m_dbusAdaptor(new MachineDBusAdaptor(m_manager, this, m_bus, m_dbusPath))
    , m_clipboard(clipboard)
    , m_dataDir(dataDir)
    , m_mountpoint(m_dataDir / "mp")
    , m_port(port)
    , m_uuid(sp.uuid())
    , m_name(sp.name())
    , m_connected(false)
    , m_os(sp.os())
    , m_compositor(sp.compositor())
    , m_deviceSharing(false)
    , m_direction(FLOW_DIRECTION_RIGHT)
    , m_pingTimer(new QTimer(this))
    , m_offlineTimer(new QTimer(this))
    , m_pairTimeoutTimer(new QTimer(this))
    , m_currentSendTransferId(0)
    , m_mounted(false)
    , m_conn(nullptr)
    , m_ip(ip) {

    m_inputEmitters.emplace(
        std::make_pair(InputDeviceType::KEYBOARD,
                       std::make_unique<InputEmitterWrapper>(InputDeviceType::KEYBOARD)));
    m_inputEmitters.emplace(
        std::make_pair(InputDeviceType::MOUSE,
                       std::make_unique<InputEmitterWrapper>(InputDeviceType::MOUSE)));
    m_inputEmitters.emplace(
        std::make_pair(InputDeviceType::TOUCHPAD,
                       std::make_unique<InputEmitterWrapper>(InputDeviceType::TOUCHPAD)));

    QObject::connect(m_pingTimer, &QTimer::timeout, this, &Machine::ping);
    m_pingTimer->start(U10s);

    QObject::connect(m_offlineTimer, &QTimer::timeout, this, &Machine::onOffline);
    m_offlineTimer->setSingleShot(true);
    m_offlineTimer->start(U25s);

    initPairRequestTimer();

    if (!m_bus.registerObject(m_dbusPath, this)) {
        qWarning() << "Failed to register request object for" << m_dbusPath << ":"
                   << m_bus.lastError().message();
        return;
    }
}

Machine::~Machine() {
    if (m_conn) {
        m_conn->close();
        m_manager->onStopDeviceSharing();
    }

    m_bus.unregisterObject(m_dbusPath);
}

const QString &Machine::path() const {
    return m_dbusPath;
}

bool Machine::isPcMachine() const {
    return m_os == DEVICE_OS_UOS || m_os == DEVICE_OS_LINUX || m_os == DEVICE_OS_WINDOWS ||
           m_os == DEVICE_OS_MACOS;
}

bool Machine::isLinux() const {
    return m_os == DEVICE_OS_LINUX || m_os == DEVICE_OS_UOS;
}

bool Machine::isAndroid() const {
    return m_os == DEVICE_OS_ANDROID;
}

void Machine::connect() {
    m_conn = new QTcpSocket(this);

    QObject::connect(m_conn, &QTcpSocket::connected, [this]() {
        qInfo() << "connected";

        initConnection();

        m_pingTimer->stop();
        m_offlineTimer->stop();

        sendPairRequest();

        m_pairTimeoutTimer->start(getPairTimeoutInterval() * 1000);
    });
    QObject::connect(m_conn, &QTcpSocket::errorOccurred, [this](QAbstractSocket::SocketError err) {
        qWarning() << "connect failed:" << err;

        // TODO tips and send scan
        m_manager->ping(m_ip);
    });
    m_conn->connectToHost(QHostAddress(QString::fromStdString(m_ip)), m_port);
}

void Machine::updateMachineInfo(const std::string &ip, uint16_t port, const DeviceInfo &devInfo) {
    m_ip = ip;
    m_port = port;
    m_compositor = devInfo.compositor();

    if (devInfo.name() != m_name) {
        m_name = devInfo.name();
        m_dbusAdaptor->updateName(QString::fromStdString(devInfo.name()));
    }
}

void Machine::receivedPing() {
    m_offlineTimer->start();
    m_pingTimer->start();
}

void Machine::onPair(QTcpSocket *socket) {
    qDebug("request onPair");
    m_conn = socket;

    auto *confirmDialog = new ConfirmDialog(QString::fromStdString(m_ip),
                                            QString::fromStdString(m_name));
    confirmDialog->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(confirmDialog,
                     &ConfirmDialog::onConfirmed,
                     this,
                     &Machine::receivedUserConfirm);
    confirmDialog->show();
}

void Machine::disconnect() {
    m_conn->close();
}

void Machine::requestDeviceSharing() {
    Message msg;
    msg.mutable_devicesharingstartrequest();
    sendMessage(msg);
}

void Machine::stopDeviceSharing() {
    Message msg;
    msg.mutable_devicesharingstoprequest();
    sendMessage(msg);

    stopDeviceSharingAux();
}

void Machine::setFlowDirection(FlowDirection direction) {
    if (m_direction != direction) {
        m_direction = (FlowDirection)direction;
        sendFlowDirectionNtf();

        m_dbusAdaptor->updateDirection(direction);
    }
}

void Machine::ping() {
    m_manager->ping(m_ip);
}

void Machine::onOffline() {
    m_manager->onMachineOffline(m_uuid);
}

void Machine::initConnection() {
    QObject::connect(m_conn, &QTcpSocket::disconnected, this, &Machine::handleDisconnectedAux);
    QObject::connect(m_conn, &QTcpSocket::readyRead, this, &Machine::dispatcher);
    m_conn->setSocketOption(QAbstractSocket::LowDelayOption, true);
    Net::tcpSocketSetKeepAliveOption(m_conn->socketDescriptor());
}

void Machine::initPairRequestTimer() {
    m_pairTimeoutTimer->setSingleShot(true);

    QObject::connect(m_pairTimeoutTimer, &QTimer::timeout, this, [this]() {
        auto *reconnectDialog = new ReconnectDialog(QString::fromStdString(m_name));
        reconnectDialog->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(reconnectDialog,
                         &ReconnectDialog::onOperated,
                         this,
                         &Machine::receivedUserOperated);
        reconnectDialog->show();
    });
}

void Machine::handleDisconnectedAux() {
    qInfo("disconnected");

    if (m_connected) {
        m_manager->onStopDeviceSharing();

        m_deviceSharing = false;
        m_dbusAdaptor->updateDeviceSharing(m_deviceSharing);
        m_connected = false;
        m_dbusAdaptor->updateConnected(m_connected);
    }

    if (m_fuseClient) {
        m_fuseClient->exit();
        m_fuseClient.reset();
    }

    if (m_fuseServer) {
        m_fuseServer.reset();
    }

    m_conn->deleteLater();
    m_conn = nullptr;

    m_pingTimer->start();
    m_offlineTimer->start();

    handleDisconnected();
}

void Machine::dispatcher() noexcept {
    qInfo() << fmt::format("received packet from name: {}, UUID: {}, size: {}",
                           std::string(m_name),
                           std::string(m_uuid),
                           m_conn->size())
                   .data();

    while (m_conn && m_conn->size() >= header_size) {
        QByteArray buffer = m_conn->peek(header_size);
        auto header = MessageHelper::parseMessageHeader(buffer);
        if (!header.legal()) {
            qWarning() << "illegal message from " << m_conn->peerAddress().toString();
            return;
        }

        if (m_conn->size() < static_cast<qint64>(header_size + header.size())) {
            qDebug() << "partial content";
            return;
        }

        m_conn->read(header_size);
        auto size = header.size();
        buffer = m_conn->read(size);

        Message msg = MessageHelper::parseMessageBody<Message>(buffer.data(), buffer.size());

        qDebug() << "message type:" << msg.payload_case();

        switch (msg.payload_case()) {
        case Message::PayloadCase::kPairResponse: {
            handlePairResponseAux(msg.pairresponse());
            break;
        }

        case Message::PayloadCase::kServiceOnOffNotification: {
            handleServiceOnOffMsg(msg.serviceonoffnotification());
            break;
        }

        case Message::PayloadCase::kDeviceSharingStartRequest: {
            handleDeviceSharingStartRequest();
            break;
        }

        case Message::PayloadCase::kDeviceSharingStartResponse: {
            handleDeviceSharingStartResponse(msg.devicesharingstartresponse());
            break;
        }

        case Message::PayloadCase::kDeviceSharingStopRequest: {
            handleDeviceSharingStopRequest();
            break;
        }

        case Message::PayloadCase::kDeviceSharingStopResponse: {
            break;
        }

        case Message::PayloadCase::kInputEventRequest: {
            handleInputEventRequest(msg.inputeventrequest());
            break;
        }

        case Message::PayloadCase::kInputEventResponse: {
            break;
        }

        case Message::PayloadCase::kFlowDirectionNtf: {
            handleFlowDirectionNtf(msg.flowdirectionntf());
            break;
        }

        case Message::PayloadCase::kFlowRequest: {
            handleFlowRequest(msg.flowrequest());
            break;
        }

        case Message::PayloadCase::kFlowResponse: {
            break;
        }

        case Message::PayloadCase::kFsRequest: {
            handleFsRequest(msg.fsrequest());
            break;
        }

        case Message::PayloadCase::kFsResponse: {
            handleFsResponse(msg.fsresponse());
            break;
        }

        case Message::PayloadCase::kFsSendFileRequest: {
            handleFsSendFileRequest(msg.fssendfilerequest());
            break;
        }

        case Message::PayloadCase::kFsSendFileResponse: {
            break;
        }

        case Message::PayloadCase::kFsSendFileResult: {
            handleFsSendFileResult(msg.fssendfileresult());
            break;
        }

        case Message::PayloadCase::kTransferRequest: {
            handleTransferRequest(msg.transferrequest());
            break;
        }

        case Message::PayloadCase::kTransferResponse: {
            handleTransferResponse(msg.transferresponse());
            break;
        }

        case Message::PayloadCase::kStopTransferRequest: {
            handleStopTransferRequest(msg.stoptransferrequest());
            break;
        }

        case Message::PayloadCase::kStopTransferResponse: {
            handleStopTransferResponse(msg.stoptransferresponse());
            break;
        }

        case Message::PayloadCase::kClipboardNotify: {
            handleClipboardNotify(msg.clipboardnotify());
            break;
        }

        case Message::PayloadCase::kClipboardGetContentRequest: {
            handleClipboardGetContentRequest(msg.clipboardgetcontentrequest());
            break;
        }

        case Message::PayloadCase::kClipboardGetContentResponse: {
            handleClipboardGetContentResponse(msg.clipboardgetcontentresponse());
            break;
        }

        case Message::PayloadCase::kCastRequest: {
            handleCastRequest(msg.castrequest());
            break;
        }

        default: {
            qWarning() << "Machine unknown message type:" << msg.payload_case();
            break;
        }
        }
    }
}

void Machine::handlePairResponseAux(const PairResponse &resp) {
    m_pairTimeoutTimer->stop();

    bool agree = resp.agree();

    // send notification
    QString msgBody;
    if (agree) {
        msgBody = QString(QObject::tr(R"RAW(Successfully connected to "%1")RAW"))
                      .arg(QString::fromStdString(m_name));
    } else {
        msgBody = QString(QObject::tr("Your connection request has been declined"));
    }

    sendReceivedFilesSystemNtf(msgBody);

    if (!agree) {
        // handle not agree
        m_conn->close();
        // rejected, need notify,ui can reset connecting status
        m_connected = false;
        m_dbusAdaptor->updateConnected(m_connected);
        return;
    }

    // connect only one machine
    if ((isPcMachine() && m_manager->hasPcMachinePaired()) ||
        (isAndroid() && m_manager->hasAndroidPaired())) {
        m_conn->close();
        m_connected = false;
        m_dbusAdaptor->updateConnected(m_connected);
        return;
    }

    m_connected = true;
    m_dbusAdaptor->updateConnected(m_connected);

    sendServiceStatusNotification();
    handleConnected();
}

void Machine::handleServiceOnOffMsg(const ServiceOnOffNotification &notification) {
    m_sharedClipboard = notification.sharedclipboardon();
}

void Machine::handleDeviceSharingStartRequest() {
    bool accepted = true;

    Message msg;
    DeviceSharingStartResponse *resp = msg.mutable_devicesharingstartresponse();
    resp->set_accept(accepted);
    sendMessage(msg);

    if (accepted) {
        auto wptr = weak_from_this();
        m_manager->onStartDeviceSharing(wptr, true);

        m_deviceSharing = true;
        m_dbusAdaptor->updateDeviceSharing(m_deviceSharing);

        m_manager->machineCooperated(m_uuid);

        m_direction = FLOW_DIRECTION_LEFT;
        m_dbusAdaptor->updateDirection(m_direction);
    }
}

void Machine::handleDeviceSharingStartResponse(const DeviceSharingStartResponse &resp) {
    if (!resp.accept()) {
        return;
    }

    m_deviceSharing = true;
    m_dbusAdaptor->updateDeviceSharing(m_deviceSharing);

    m_manager->machineCooperated(m_uuid);

    m_direction = FLOW_DIRECTION_RIGHT;
    m_dbusAdaptor->updateDirection(m_direction);

    sendFlowDirectionNtf();

    m_manager->onStartDeviceSharing(weak_from_this(), true);
}

void Machine::handleDeviceSharingStopRequest() {
    stopDeviceSharingAux();
}

void Machine::handleInputEventRequest(const InputEventRequest &req) {
    qDebug("received input event");

    bool success = true;

    auto deviceType = static_cast<InputDeviceType>(req.devicetype());
    auto it = m_inputEmitters.find(deviceType);
    if (it == m_inputEmitters.end()) {
        success = false;
        qWarning()
            << fmt::format("no deviceType {} found", static_cast<uint8_t>(deviceType)).data();
    } else {
        auto &inputEmitter = it->second;
        success = inputEmitter->emitEvent(req.type(), req.code(), req.value());
    }

    Message resp;
    InputEventResponse *response = resp.mutable_inputeventresponse();
    response->set_serial(req.serial());
    response->set_success(success);

    m_conn->write(MessageHelper::genMessage(resp));
}

void Machine::handleFlowDirectionNtf(const FlowDirectionNtf &ntf) {
    FlowDirection peerFlowDirection = ntf.direction();
    switch ((int)peerFlowDirection) {
    case FLOW_DIRECTION_TOP:
        m_direction = FLOW_DIRECTION_BOTTOM;
        break;
    case FLOW_DIRECTION_BOTTOM:
        m_direction = FLOW_DIRECTION_TOP;
        break;
    case FLOW_DIRECTION_LEFT:
        m_direction = FLOW_DIRECTION_RIGHT;
        break;
    case FLOW_DIRECTION_RIGHT:
        m_direction = FLOW_DIRECTION_LEFT;
        break;
    }

    m_dbusAdaptor->updateDirection(m_direction);
}

void Machine::handleFlowRequest(const FlowRequest &req) {
    m_manager->onFlowBack(req.direction(), req.x(), req.y());
}

void Machine::handleFsRequest([[maybe_unused]] const FsRequest &req) {
    if (m_fuseServer) {
        Message msg;
        auto *fsresponse = msg.mutable_fsresponse();
        fsresponse->set_accepted(false);
        fsresponse->set_port(0);
        sendMessage(msg);
        return;
    }

    m_fuseServer = std::make_unique<FuseServer>(weak_from_this());

    // TODO: request accept
    Message msg;
    auto *fsresponse = msg.mutable_fsresponse();
    fsresponse->set_accepted(true);
    fsresponse->set_port(m_fuseServer->port());
    sendMessage(msg);
}

void Machine::handleFsResponse(const FsResponse &resp) {
    if (!resp.accepted()) {
        return;
    }

    m_fuseClient = std::make_unique<FuseClient>(m_ip, resp.port(), m_mountpoint);
}

void Machine::handleFsSendFileRequest(const FsSendFileRequest &req) {
    Message msg;
    auto *fssendfileresponse = msg.mutable_fssendfileresponse();
    fssendfileresponse->set_serial(req.serial());

    if (!m_fuseClient) {
        fssendfileresponse->set_accepted(false);
        sendMessage(msg);
        return;
    }

    fssendfileresponse->set_accepted(true);
    sendMessage(msg);

    QString storagePath = m_manager->fileStoragePath();
    std::string reqPath = req.path();
    if (!reqPath.empty() && reqPath[0] != '/') {
        reqPath = "/" + reqPath;
    }
    std::string filePath = m_mountpoint.string() + reqPath;
    QProcess *process = new QProcess(this);
    QObject::connect(
        process,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        [this,
         storagePath = storagePath.toStdString(),
         serial = req.serial(),
         path = req.path(),
         process]([[maybe_unused]] int exitCode, QProcess::ExitStatus exitStatus) {
            Message msg;
            auto *fssendfileresult = msg.mutable_fssendfileresult();
            fssendfileresult->set_serial(serial);
            fssendfileresult->set_path(path);

            if (exitStatus != QProcess::NormalExit) {
                qInfo("copy files failed");
            } else {
                qInfo("copy files success");
            }

            std::string::size_type iPos = path.find_last_of('/') + 1;
            std::string fileName = path.substr(iPos, path.length() - iPos);
            QString msgBody;
            if (exitStatus == QProcess::NormalExit) {
                msgBody = QString(QObject::tr(R"RAW(Successfully received files from "%1")RAW"))
                              .arg(QString::fromStdString(m_name));
            } else {
                msgBody = QString(QObject::tr(R"RAW(Failed to receive files from "%1")RAW"))
                              .arg(QString::fromStdString(m_name));
            }

            sendReceivedFilesSystemNtf(msgBody);

            fssendfileresult->set_result(exitStatus == QProcess::NormalExit);
            sendMessage(msg);

            process->deleteLater();
        });
    process->start("/bin/cp", QStringList{"-r", QString::fromStdString(filePath), storagePath});
}

void Machine::handleFsSendFileResult(const FsSendFileResult &resp) {
    QString msgBody;
    if (resp.result()) {
        msgBody = QString(QObject::tr(R"RAW(Successfully sent to "%1")RAW"))
                      .arg(QString::fromStdString(m_name));
    } else {
        msgBody = QString(QObject::tr(R"RAW(Failed to send files to"%1")RAW"))
                      .arg(QString::fromStdString(m_name));
    }

    sendReceivedFilesSystemNtf(msgBody);
}

void Machine::handleTransferRequest(const TransferRequest &req) {
    auto *transfer = new ReceiveTransfer(m_manager->getFileStoragePath().toStdString(), this);
    m_receiveTransfers.emplace(transfer);

    QObject::connect(transfer, &ReceiveTransfer::destroyed, this, [this, transfer]() {
        m_receiveTransfers.erase(transfer);
    });

    uint32_t transferId = req.transferid();
    Message msg;
    auto *transferResponse = msg.mutable_transferresponse();
    transferResponse->set_transferid(transferId);
    transferResponse->set_accepted(true);
    transferResponse->set_port(transfer->port());
    sendMessage(msg);
}

void Machine::handleTransferResponse(const TransferResponse &resp) {
    uint32_t transferId = resp.transferid();

    if (!resp.accepted()) {
        // TODO:
        m_sendTransfers.erase(transferId);
        return;
    }

    auto iter = m_sendTransfers.find(transferId);
    if (iter == m_sendTransfers.end()) {
        return;
    }

    auto [_, transfer] = *iter;

    transfer->send(m_ip, resp.port());
}

void Machine::handleStopTransferRequest(const StopTransferRequest &req) {
    Message msg;
    auto *stopTransferResponse = msg.mutable_stoptransferresponse();
    stopTransferResponse->set_transferid(req.transferid());
}

void Machine::handleStopTransferResponse(const StopTransferResponse &resp) {
    auto transferId = resp.transferid();
    auto iter = m_sendTransfers.find(transferId);
    if (iter == m_sendTransfers.end()) {
        return;
    }

    auto [_, transfer] = *iter;
    transfer->stop();
}

void Machine::handleClipboardNotify(const ClipboardNotify &notify) {
    auto &targetsp = notify.targets();
    std::vector<std::string> targets{targetsp.cbegin(), targetsp.cend()};

    // other pc machine need fill up text/uri-list target
    if (m_os != DEVICE_OS_UOS &&
        std::find(targets.begin(), targets.end(), clipboardFileTarget) != targets.end()) {
        targets.emplace_back(uriListTarget);
    }

    m_manager->onMachineOwnClipboard(weak_from_this(), targets);
}

void Machine::handleClipboardGetContentRequest(const ClipboardGetContentRequest &req) {
    auto target = req.target();
    auto cb = [this, target](const std::vector<char> &content) {
        Message msg;
        auto *reply = msg.mutable_clipboardgetcontentresponse();
        reply->set_target(target);
        reply->set_content(std::string(content.begin(), content.end()));
        sendMessage(msg);
    };
    m_clipboard->readTargetContent(target, cb);
}

void Machine::handleClipboardGetContentResponse(const ClipboardGetContentResponse &resp) {
    auto target = resp.target();
    auto content = resp.content();
    if (target == "x-special/gnome-copied-files") {
        qDebug() << fmt::format("ori x-special/gnome-copied-files: {}", content).data();
    }
    if (m_clipboard->isFiles()) {
        std::stringstream ss(content);
        std::string out;
        std::string line;
        while (!ss.eof()) {
            std::getline(ss, line, '\n');
            if (line[0] == '/') { // starts with '/'
                out.reserve(out.length() + m_mountpoint.string().length() + line.length() + 1);
                out.append(m_mountpoint.string());
                out.append(line);
            } else if (line.rfind(fileSchema, 0) == 0) { // starts with 'file://'
                out.reserve(out.length() + m_mountpoint.string().length() + line.length() + 1);
                out.append(fileSchema);
                out.append(m_mountpoint.string());
                out.append(line.begin() + fileSchema.length(), line.end());
            } else {
                out.reserve(out.length() + line.length() + 1);
                out.append(line);
            }
            out.push_back('\n');
        }
        out.resize(out.length() - 1);
        content.swap(out);
        // qInfo() << fmt::format("content[{}]: {}", content.length(), content).data();
    }

    // fill up text/uri-list target. when pasted, this target is need;
    if (m_os != DEVICE_OS_UOS && target == clipboardFileTarget) {
        std::stringstream tempStream(content);
        std::string filePath;
        while (!tempStream.eof()) {
            std::getline(tempStream, filePath, '\n');
            if (!filePath.empty() && filePath.rfind(fileSchema, 0) == 0) { // starts with 'file://'
                filePath = std::string(filePath.data() + fileSchema.size(),
                                       filePath.size() - fileSchema.size());
                break;
            }
        }

        if (!filePath.empty()) {
            qDebug() << fmt::format("pc machine fill up text/uri-list target: {}", filePath).data();
            m_clipboard->updateTargetContent(uriListTarget,
                                             std::vector<char>(filePath.begin(), filePath.end()));
        }
    }

    m_clipboard->updateTargetContent(target, std::vector<char>(content.begin(), content.end()));
}

void Machine::onInputGrabberEvent(uint8_t deviceType,
                                  unsigned int type,
                                  unsigned int code,
                                  int value) {
    Message msg;
    auto *inputEvent = msg.mutable_inputeventrequest();
    inputEvent->set_devicetype(static_cast<DeviceType>(deviceType));
    inputEvent->set_type(type);
    inputEvent->set_code(code);
    inputEvent->set_value(value);
    sendMessage(msg);
}

void Machine::onClipboardTargetsChanged(const std::vector<std::string> &targets) {
    // sharedClipboard off, not send msg
    if (!m_manager->isSharedClipboard()) {
        return;
    }

    Message msg;
    auto *clipboardNotify = msg.mutable_clipboardnotify();
    *(clipboardNotify->mutable_targets()) = {targets.cbegin(), targets.cend()};
    sendMessage(msg);
}

void Machine::flowTo(uint16_t direction, uint16_t x, uint16_t y) noexcept {
    Message msg;
    FlowRequest *flow = msg.mutable_flowrequest();
    flow->set_direction(FlowDirection(direction));
    flow->set_x(x);
    flow->set_y(y);
    sendMessage(msg);
}

void Machine::readTarget(const std::string &target) {
    Message msg;
    auto *clipboardGetContent = msg.mutable_clipboardgetcontentrequest();
    clipboardGetContent->set_target(target);
    sendMessage(msg);
}

void Machine::stopDeviceSharingAux() {
    m_manager->onStopDeviceSharing();

    m_deviceSharing = false;
    m_dbusAdaptor->updateDeviceSharing(m_deviceSharing);
}

void Machine::receivedUserConfirm(bool accepted) {
    Message msg;
    auto *response = msg.mutable_pairresponse();
    response->set_key(SCAN_KEY);
    m_manager->completeDeviceInfo(response->mutable_deviceinfo());
    response->set_agree(accepted); // 询问用户是否同意

    sendMessage(msg);

    // send notification
    if (accepted) {
        QString msgBody;
        msgBody = QString(QObject::tr(R"RAW(Successfully connected to "%1")RAW"))
                      .arg(QString::fromStdString(m_name));
        sendReceivedFilesSystemNtf(msgBody);
    }

    if (accepted) {
        initConnection();

        m_pingTimer->stop();
        m_offlineTimer->stop();

        m_connected = true;
        m_dbusAdaptor->updateConnected(m_connected);

        sendServiceStatusNotification();
        handleConnected();
    } else {
        m_conn->close();
    }
}

void Machine::receivedUserOperated(bool tryAgain) {
    if (tryAgain) {
        sendPairRequest();
        m_pairTimeoutTimer->start(getPairTimeoutInterval() * 1000);
    } else {
        m_conn->close();
    }
}

void Machine::sendFlowDirectionNtf() {
    Message msg;
    auto *notification = msg.mutable_flowdirectionntf();
    notification->set_direction((FlowDirection)m_direction);
    sendMessage(msg);
}

void Machine::sendReceivedFilesSystemNtf(const QString &body) {
    DDBusSender()
        .service("org.freedesktop.Notifications")
        .path("/org/freedesktop/Notifications")
        .interface("org.freedesktop.Notifications")
        .method(QString("Notify"))
        .arg(QString("notification_collaboration"))
        .arg(static_cast<uint>(0))
        .arg(QString(""))
        .arg(QObject::tr("PC Collaboration"))
        .arg(body)
        .arg(QStringList())
        .arg(QVariantMap())
        .arg(5000)
        .call();
}

int Machine::getPairTimeoutInterval() {
    int interval = 60; // default

    DConfig *dConfigPtr = DConfig::create(dConfigAppID, dConfigName);
    if (dConfigPtr && dConfigPtr->isValid() && dConfigPtr->keyList().contains("timeoutInterval")) {
        interval = dConfigPtr->value("timeoutInterval").toInt();
    }

    dConfigPtr->deleteLater();

    return interval;
}

void Machine::sendPairRequest() {
    Message msg;
    auto *request = msg.mutable_pairrequest();
    request->set_key(SCAN_KEY);
    m_manager->completeDeviceInfo(request->mutable_deviceinfo());

    sendMessage(msg);
}

void Machine::transferSendFiles(const QStringList &filePaths) {
    m_currentSendTransferId++;
    uint32_t transferId = m_currentSendTransferId;
    auto *transfer = new SendTransfer(filePaths, this);
    m_sendTransfers.emplace(transferId, transfer);

    QObject::connect(transfer, &SendTransfer::done, this, [this, transferId]() {
        Message msg;
        auto *stopSendTransferRequest = msg.mutable_stoptransferrequest();
        stopSendTransferRequest->set_transferid(transferId);

        m_conn->write(MessageHelper::genMessage(msg));
    });
    QObject::connect(transfer, &SendTransfer::destroyed, this, [this, transferId]() {
        m_sendTransfers.erase(transferId);
    });

    Message msg;
    auto *transferRequest = msg.mutable_transferrequest();
    transferRequest->set_transferid(transferId);

    sendMessage(msg);
}

void Machine::sendMessage(const Message &msg) {
    if (!m_conn) {
        qWarning() << fmt::format("connection reset but still want to send msg: {}",
                                  msg.GetTypeName())
                          .data();
        return;
    }

    m_conn->write(MessageHelper::genMessage(msg));
}

void Machine::sendServiceStatusNotification() {
    Message msg;
    auto *notification = msg.mutable_serviceonoffnotification();
    notification->set_sharedclipboardon(m_manager->isSharedClipboard());
    notification->set_shareddeviceson(m_manager->isSharedDevices());

    sendMessage(msg);
}