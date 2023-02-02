// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>

#include <QLocalSocket>

#include "InputEmitter.h"

#include "utils/message_helper.h"
#include "protocol/ipc_message.pb.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setSetuidAllowed(true);
    QCoreApplication app(argc, argv);

    auto args = app.arguments();
    if (args.size() < 3) {
        qWarning("3 args");
        return 1;
    }

    auto addr = args[1];
    qDebug() << "input-emitter LocalServer addr:" << addr;

    InputDeviceType type = static_cast<InputDeviceType>(args[2].toUInt());
    InputEmitter emitter(type);

    QLocalSocket socket;
    QObject::connect(&socket, &QLocalSocket::readyRead, [&emitter, &socket]() {
        while (socket.size() >= header_size) {
            QByteArray buffer = socket.peek(header_size);
            auto header = MessageHelper::parseMessageHeader(buffer);
            if (!header.legal()) {
                qWarning() << "illegal message from InputEmitterWrapper";
                return;
            }

            if (socket.size() < static_cast<qint64>(header_size + header.size())) {
                qDebug() << "partial content";
                return;
            }

            socket.read(header_size);
            auto size = header.size();
            buffer = socket.read(size);

            auto base = MessageHelper::parseMessageBody<InputEmitterParent>(buffer.data(),
                                                                            buffer.size());

            switch (base.payload_case()) {
            case InputEmitterParent::PayloadCase::kInputEvent: {
                auto &inputEvent = base.inputevent();
                emitter.emitEvent(inputEvent.type(), inputEvent.code(), inputEvent.value());
            } break;
            case InputEmitterParent::PayloadCase::PAYLOAD_NOT_SET: {
            } break;
            }
        }
    });
    QObject::connect(&socket, &QLocalSocket::disconnected, [&app]() { app.quit(); });
    socket.connectToServer(addr);

    return app.exec();
}
