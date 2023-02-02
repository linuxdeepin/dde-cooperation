// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>
#include <QLocalSocket>

#include "InputGrabber.h"

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
    qDebug() << "input-grabber LocalServer addr:" << addr;
    auto path = args[2];

    InputGrabber grabber(path.toStdString());
    if (grabber.shouldIgnore()) {
        return 2;
    }

    QLocalSocket socket;
    QObject::connect(&socket, &QLocalSocket::readyRead, [&grabber, &socket]() {
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

            auto base = MessageHelper::parseMessageBody<InputGrabberParent>(buffer.data(),
                                                                            buffer.size());

            switch (base.payload_case()) {
            case InputGrabberParent::PayloadCase::kStart: {
                grabber.start();
            } break;
            case InputGrabberParent::PayloadCase::kStop: {
                grabber.stop();
            } break;
            case InputGrabberParent::PayloadCase::PAYLOAD_NOT_SET: {
            } break;
            }
        }
    });
    QObject::connect(&socket, &QLocalSocket::connected, [&grabber, &socket]() {
        InputGrabberChild msg;
        msg.set_devicetype(static_cast<uint8_t>(grabber.type()));
        socket.write(MessageHelper::genMessage(msg));
    });
    QObject::connect(&socket, &QLocalSocket::disconnected, [&app]() { app.quit(); });
    socket.connectToServer(addr);

    grabber.onEvent([&socket](unsigned int type, unsigned int code, int value) {
        InputGrabberChild msg;
        auto *inputEvent = msg.mutable_inputevent();
        inputEvent->set_type(type);
        inputEvent->set_code(code);
        inputEvent->set_value(value);
        socket.write(MessageHelper::genMessage(msg));
    });

    return app.exec();
}
