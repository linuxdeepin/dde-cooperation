// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "datadirectories.h"

namespace sslconf {

fs::path DataDirectories::_profile;
fs::path DataDirectories::_global;
fs::path DataDirectories::_systemconfig;

static const char kFingerprintsDirName[] = "SSL/Fingerprints";
static const char kFingerprintsLocalFilename[] = "Local.txt";
static const char kFingerprintsTrustedServersFilename[] = "TrustedServers.txt";
static const char kFingerprintsTrustedClientsFilename[] = "TrustedClients.txt";

fs::path DataDirectories::ssl_fingerprints_path()
{
    return profile() / kFingerprintsDirName;
}

fs::path DataDirectories::local_ssl_fingerprints_path()
{
    return ssl_fingerprints_path() / kFingerprintsLocalFilename;
}

fs::path DataDirectories::trusted_servers_ssl_fingerprints_path()
{
    return ssl_fingerprints_path() / kFingerprintsTrustedServersFilename;
}

fs::path DataDirectories::trusted_clients_ssl_fingerprints_path()
{
    return ssl_fingerprints_path() / kFingerprintsTrustedClientsFilename;
}

fs::path DataDirectories::ssl_certificate_path()
{
    return profile() / "SSL" / "Barrier.pem";
}

} // namespace sslconf
