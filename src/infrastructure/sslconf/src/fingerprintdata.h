// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_FINGERPRINT_DATA_H
#define SSLCONF_FINGERPRINT_DATA_H

#include <string>
#include <vector>
#include <cstdint>

namespace sslconf {

enum FingerprintType {
    INVALID,
    SHA1, // deprecated
    SHA256,
};

struct FingerprintData {
    std::string algorithm;
    std::vector<std::uint8_t> data;

    bool valid() const { return !algorithm.empty(); }

    bool operator==(const FingerprintData& other) const;
};

const char* fingerprint_type_to_string(FingerprintType type);
FingerprintType fingerprint_type_from_string(const std::string& type);

} // namespace sslconf

#endif
