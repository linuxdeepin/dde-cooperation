// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef KEYPAIR_H
#define KEYPAIR_H

#include <filesystem>

class KeyPair {
public:
    enum class KeyType {
        ED25519,
    };

    explicit KeyPair(const std::filesystem::path &dataDir, KeyType type);

    bool load();

    std::string getFingerprint() const { return m_fingerprint; }

private:
    KeyType m_type;

    std::filesystem::path m_privatePath;
    std::filesystem::path m_publicPath;

    std::string m_fingerprint;

    bool generateNewKey();
};

#endif // !KEYPAIR_H
