// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SECURECONFIG_H
#define SECURECONFIG_H

#include "asio/ssl_context.h"

#include <string>

class SecureConfig {
public:
    // Android has no QStandardPaths. The app passes its private files
    // directory here before any context is created.
    static void setConfigDir(const std::string &dir);

    static std::shared_ptr<NetUtil::Asio::SSLContext> serverContext();

    static std::shared_ptr<NetUtil::Asio::SSLContext> clientContext();

private:
};

#endif // SECURECONFIG_H
