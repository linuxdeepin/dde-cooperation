// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "confstring.h"
#include "fingerprintdatabase.h"
#include "filesystem.h"
#include <algorithm>
#include <fstream>

namespace sslconf {

bool FingerprintData::operator==(const FingerprintData& other) const
{
    return algorithm == other.algorithm && data == other.data;
}

const char* fingerprint_type_to_string(FingerprintType type)
{
    switch (type) {
        case FingerprintType::INVALID: return "invalid";
        case FingerprintType::SHA1: return "sha1";
        case FingerprintType::SHA256: return "sha256";
    }
    return "invalid";
}

FingerprintType fingerprint_type_from_string(const std::string& type)
{
    if (type == "sha1") {
        return FingerprintType::SHA1;
    }
    if (type == "sha256") {
        return FingerprintType::SHA256;
    }
    return FingerprintType::INVALID;
}

} // namespace sslconf
