// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "secureconfig.h"

#include "sslcertconf.h"

#include <QStandardPaths>

static std::string getCertFilePath()
{
    auto configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).toStdString();
    SslCertConf::ins()->generateCertificate(configDir);
    return SslCertConf::ins()->getCertificatePath();
}


std::shared_ptr<NetUtil::Asio::SSLContext> SecureConfig::serverContext()
{
    auto certfile = getCertFilePath();

    auto context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv13);
    context->use_certificate_file(certfile, asio::ssl::context::pem);
    context->use_rsa_private_key_file(certfile, asio::ssl::context::pem);

    return context;
}


std::shared_ptr<NetUtil::Asio::SSLContext> SecureConfig::clientContext()
{
    auto certfile = getCertFilePath();

    auto context = std::make_shared<NetUtil::Asio::SSLContext>(asio::ssl::context::tlsv13);
    context->use_certificate_file(certfile, asio::ssl::context::pem);

    return context;
}
