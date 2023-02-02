// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "InputEmitterWrapper.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>

#include "config.h"
#include "utils/message_helper.h"
#include "protocol/ipc_message.pb.h"

InputEmitterWrapper::InputEmitterWrapper(InputDeviceType type)
    : m_server(new QLocalServer(this))
    , m_conn(nullptr)
    , m_process(new QProcess(this))
    , m_type(type) {
    m_server->listen(QString("DDECooperationInputEmitter-%1-%2")
                         .arg(reinterpret_cast<quintptr>(this))
                         .arg(static_cast<uint8_t>(type)));
    m_server->setMaxPendingConnections(1);
    qDebug() << "InputEmitter listen addr:" << m_server->serverName();

    connect(m_server,
            &QLocalServer::newConnection,
            this,
            &InputEmitterWrapper::handleNewConnection);

    connect(m_process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this,
            &InputEmitterWrapper::onProcessClosed);
    m_process->setProcessChannelMode(QProcess::ForwardedChannels);
    m_process->start(
        INPUT_EMITTER_PATH,
        QStringList{m_server->serverName(), QString::number(static_cast<uint8_t>(type))});
}

InputEmitterWrapper::~InputEmitterWrapper() {
    m_server->close();
    if (m_conn) {
        m_conn->close();
    }
    m_process->kill();
    m_process->waitForFinished(100);
}

bool InputEmitterWrapper::emitEvent(unsigned int type, unsigned int code, int value) noexcept {
    if (!m_conn) {
        return false;
    }

    InputEmitterParent msg;
    auto *inputEvent = msg.mutable_inputevent();
    inputEvent->set_type(type);
    inputEvent->set_code(code);
    inputEvent->set_value(value);
    return m_conn->write(MessageHelper::genMessage(msg));
}

void InputEmitterWrapper::handleNewConnection() {
    if (m_conn) {
        return;
    }

    m_conn = m_server->nextPendingConnection();
    m_server->close();

    connect(m_conn, &QLocalSocket::readyRead, this, &InputEmitterWrapper::onReceived);
    connect(m_conn, &QLocalSocket::disconnected, this, &InputEmitterWrapper::onDisconnected);
}

void InputEmitterWrapper::onReceived() {
    qDebug("input-emitter onReceived");
}

void InputEmitterWrapper::onProcessClosed([[maybe_unused]] int exitCode,
                                          [[maybe_unused]] QProcess::ExitStatus exitStatus) {
    // todo: remove emitter
}

void InputEmitterWrapper::onDisconnected() {
    // todo: remove emitter
}
