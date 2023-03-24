// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef ANDROID_QRCODEPROXY_H
#define ANDROID_QRCODEPROXY_H

#include <QObject>
#include <QrCode.hpp>

class QrCodeProxy : public QObject {
    Q_OBJECT

public:
    explicit QrCodeProxy(QObject *parent = nullptr);

    void setText(const QString &text);

    Q_INVOKABLE int getSize() const;
    Q_INVOKABLE bool getModule(int x, int y) const;

signals:
    void textChanged();

private:
    qrcodegen::QrCode m_qrcode;
};

#endif // ANDROID_QRCODEPROXY_H
