// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "confstring.h"
#include "fingerprintdatabase.h"
#include "filesystem.h"
#include <algorithm>
#include <fstream>

namespace sslconf {

void FingerprintDatabase::read(const fs::path& path)
{
    std::ifstream file;
    open_utf8_path(file, path, std::ios_base::in);
    read_stream(file);
}

void FingerprintDatabase::write(const fs::path& path)
{
    std::ofstream file;
    open_utf8_path(file, path, std::ios_base::out);
    write_stream(file);
}

void FingerprintDatabase::read_stream(std::istream& stream)
{
    if (!stream.good()) {
        return;
    }

    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        auto fingerprint = parse_db_line(line);
        if (!fingerprint.valid()) {
            continue;
        }

        fingerprints_.push_back(fingerprint);
    }
}

void FingerprintDatabase::write_stream(std::ostream& stream)
{
    if (!stream.good()) {
        return;
    }

    for (const auto& fingerprint : fingerprints_) {
        stream << to_db_line(fingerprint) << "\n";
    }
}

void FingerprintDatabase::clear()
{
    fingerprints_.clear();
}

void FingerprintDatabase::add_trusted(const FingerprintData& fingerprint)
{
    if (is_trusted(fingerprint)) {
        return;
    }
    fingerprints_.push_back(fingerprint);
}

bool FingerprintDatabase::is_trusted(const FingerprintData& fingerprint)
{
    auto found_it = std::find(fingerprints_.begin(), fingerprints_.end(), fingerprint);
    return found_it != fingerprints_.end();
}

FingerprintData FingerprintDatabase::parse_db_line(const std::string& line)
{
    FingerprintData result;

    // legacy v1 certificate handling
    if (std::count(line.begin(), line.end(), ':') == 19 && line.size() == 40 + 19) {
        auto data = string::from_hex(line);
        if (data.empty()) {
            return result;
        }
        result.algorithm = fingerprint_type_to_string(FingerprintType::SHA1);
        result.data = data;
        return result;
    }

    auto version_end_pos = line.find(':');
    if (version_end_pos == std::string::npos) {
        return result;
    }
    if (line.substr(0, version_end_pos) != "v2") {
        return result;
    }
    auto algo_start_pos = version_end_pos + 1;
    auto algo_end_pos = line.find(':', algo_start_pos);
    if (algo_end_pos == std::string::npos) {
        return result;
    }
    auto algorithm = line.substr(algo_start_pos, algo_end_pos - algo_start_pos);
    auto data = string::from_hex(line.substr(algo_end_pos + 1));

    if (data.empty()) {
        return result;
    }

    result.algorithm = algorithm;
    result.data = data;
    return result;
}

std::string FingerprintDatabase::to_db_line(const FingerprintData& fingerprint)
{
    return "v2:" + fingerprint.algorithm + ":" + string::to_hex(fingerprint.data, 2);
}

} // namespace sslconf
