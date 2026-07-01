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

class TokenCache : public BaseKit::Singleton<TokenCache>
{
    friend BaseKit::Singleton<TokenCache>;

public:
    std::string genToken(std::string info);

    bool verifyToken(std::string &token);

    std::vector<std::string> getWebfromToken(const std::string &token);

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
