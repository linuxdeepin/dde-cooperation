// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SECURECONFIG_H
#define SECURECONFIG_H

#include "asio/ssl_context.h"

class SecureConfig {
public:
    static std::shared_ptr<NetUtil::Asio::SSLContext> serverContext();

    static std::shared_ptr<NetUtil::Asio::SSLContext> clientContext();

private:
};

#endif // SECURECONFIG_H
