// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "QrCodeProxy.h"

using namespace qrcodegen;

QrCodeProxy::QrCodeProxy(QObject *parent)
    : QObject(parent)
    , m_qrcode(QrCode::encodeText("", QrCode::Ecc::LOW)) {
}

void QrCodeProxy::setText(const QString &text) {
    m_qrcode = QrCode::encodeText(text.toStdString().c_str(), QrCode::Ecc::MEDIUM);
    emit textChanged();
}

int QrCodeProxy::getSize() const {
    return m_qrcode.getSize();
}

bool QrCodeProxy::getModule(int x, int y) const {
    return m_qrcode.getModule(x, y);
}
