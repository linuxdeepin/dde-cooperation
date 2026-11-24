// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TOKENCACHE_H
#define TOKENCACHE_H

#include "string/string_utils.h"
#include "utility/singleton.h"

#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

class TokenCache : public BaseKit::Singleton<TokenCache>
{
    friend BaseKit::Singleton<TokenCache>;

public:
    // Parsed, unverified view of the optional `dtp` claim. It is only a hint
    // used to decide whether the peer speaks the compressed stream protocol;
    // the server still verifies the token signature on every request.
    struct DtpCapability {
        bool present {false};
        int version {0};
        std::vector<std::string> codecs;

        bool supportsStream() const { return present && version >= 2; }
    };

    // includeDtp=false emits a pre-DTP2-shaped token (no `dtp` claim), used
    // when the server's stream endpoint is disabled.
    std::string genToken(std::string info, bool includeDtp = true);

    bool verifyToken(std::string &token);

    std::vector<std::string> getWebfromToken(const std::string &token);

    // Returns an unsupported capability for old tokens, malformed tokens and
    // tokens whose claim is not valid JSON. Never throws.
    DtpCapability getDtpFromToken(const std::string &token) const;

    void clearTokens();

private:
    struct Es256kKeyPair {
        std::string privateKey;
        std::string publicKey;
        std::chrono::system_clock::time_point expireTime;
    };

    static Es256kKeyPair generateEs256kKeyPair();

    void cleanupExpiredTokens();

    std::mutex _cache_lock;
    std::map<std::string, Es256kKeyPair, std::less<>> _cache;
};

#endif // TOKENCACHE_H
