// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_DATA_DIRECTORIES_H
#define SSLCONF_DATA_DIRECTORIES_H

#include "filesystem.h"

namespace sslconf {

class DataDirectories
{
public:
    static const fs::path& profile();
    static const fs::path& profile(const fs::path& path);

    static const fs::path& global();
    static const fs::path& global(const fs::path& path);

    static const fs::path& systemconfig();
    static const fs::path& systemconfig(const fs::path& path);

    static fs::path ssl_fingerprints_path();
    static fs::path local_ssl_fingerprints_path();
    static fs::path trusted_servers_ssl_fingerprints_path();
    static fs::path trusted_clients_ssl_fingerprints_path();
    static fs::path ssl_certificate_path();
private:
    static fs::path _profile;
    static fs::path _global;
    static fs::path _systemconfig;
};

} // namespace sslconf

#endif
