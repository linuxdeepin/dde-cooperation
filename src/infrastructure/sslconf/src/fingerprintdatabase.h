// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SSLCONF_FINGERPRINT_DATABASE_H
#define SSLCONF_FINGERPRINT_DATABASE_H

#include "fingerprintdata.h"
#include "filesystem.h"
#include <iosfwd>
#include <string>
#include <vector>

namespace sslconf {

class FingerprintDatabase {
public:
    void read(const fs::path& path);
    void write(const fs::path& path);

    void read_stream(std::istream& stream);
    void write_stream(std::ostream& stream);

    void clear();
    void add_trusted(const FingerprintData& fingerprint);
    bool is_trusted(const FingerprintData& fingerprint);

    const std::vector<FingerprintData>& fingerprints() const { return fingerprints_; }

    static FingerprintData parse_db_line(const std::string& line);
    static std::string to_db_line(const FingerprintData& fingerprint);

private:

    std::vector<FingerprintData> fingerprints_;
};

} // namespace sslconf

#endif
