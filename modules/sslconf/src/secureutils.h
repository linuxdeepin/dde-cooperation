// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_SECUREUTILS_H
#define SSLCONF_SECUREUTILS_H

#include "fingerprintdata.h"

#include <openssl/ossl_typ.h>
#include <cstdint>
#include <string>
#include <vector>

namespace sslconf {

std::string format_ssl_fingerprint(const std::vector<std::uint8_t>& fingerprint,
                                   bool separator = true);
std::string format_ssl_fingerprint_columns(const std::vector<uint8_t>& fingerprint);

FingerprintData get_ssl_cert_fingerprint(X509* cert, FingerprintType type);

FingerprintData get_pem_file_cert_fingerprint(const std::string& path, FingerprintType type);

void generate_pem_self_signed_cert(const std::string& path);

std::string create_fingerprint_randomart(const std::vector<std::uint8_t>& dgst_raw);

} // namespace sslconf

#endif
