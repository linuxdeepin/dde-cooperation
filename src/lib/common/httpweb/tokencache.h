// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TOKENCACHE_H
#define TOKENCACHE_H

#include "string/string_utils.h"
#include "utility/singleton.h"

#include <iostream>
#include <map>
#include <mutex>

//typedef struct {
//    std::string mount_point;
//    std::string base_dir;
//} MountPointEntry;

class TokenCache : public CppCommon::Singleton<TokenCache>
{
    friend CppCommon::Singleton<TokenCache>;

public:
    std::string getAllCache();

    bool getCacheValue(std::string_view key, std::string &value);

    void putCacheValue(std::string_view key, std::string_view value);

    bool deleteCacheValue(std::string_view key, std::string &value);

    void clearCache();

//    bool setMountPoint(const std::string &mount_point, const std::string &dir);

//    bool removeMountPoint(const std::string &mount_point);

//    std::vector<MountPointEntry> getBaseDir();
private:
    std::mutex _cache_lock;
    std::map<std::string, std::string, std::less<>> _cache;


//    std::vector<MountPointEntry> _base_dirs;
};

#endif // TOKENCACHE_H
