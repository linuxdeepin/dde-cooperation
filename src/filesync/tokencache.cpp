// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tokencache.h"

std::string TokenCache::GetAllCache()
{
    std::scoped_lock locker(_cache_lock);
    std::string result;
    result += "[\n";
    for (const auto &item : _cache) {
        result += "  {\n";
        result += "    \"key\": \"" + item.first + "\",\n";
        result += "    \"value\": \"" + item.second + "\",\n";
        result += "  },\n";
    }
    result += "]\n";
    return result;
}

bool TokenCache::GetCacheValue(std::string_view key, std::string &value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.find(key);
    if (it != _cache.end()) {
        value = it->second;
        return true;
    } else
        return false;
}

void TokenCache::PutCacheValue(std::string_view key, std::string_view value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.emplace(key, value);
    if (!it.second)
        it.first->second = value;
}

bool TokenCache::DeleteCacheValue(std::string_view key, std::string &value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.find(key);
    if (it != _cache.end()) {
        value = it->second;
        _cache.erase(it);
        return true;
    } else
        return false;
}
