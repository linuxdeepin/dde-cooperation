// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tokencache.h"
#include "filesystem/path.h"

std::string TokenCache::getAllCache()
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

bool TokenCache::getCacheValue(std::string_view key, std::string &value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.find(key);
    if (it != _cache.end()) {
        value = it->second;
        return true;
    } else
        return false;
}

void TokenCache::putCacheValue(std::string_view key, std::string_view value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.emplace(key, value);
    if (!it.second)
        it.first->second = value;
}

bool TokenCache::deleteCacheValue(std::string_view key, std::string &value)
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

void TokenCache::clearCache()
{
    _cache.clear();
}

//bool TokenCache::setMountPoint(const std::string &mount_point, const std::string &dir)
//{
//    CppCommon::Path dirpath = dir;
//    if (dirpath.IsDirectory()) {
//        std::string mnt = !mount_point.empty() ? mount_point : "/";
//        if (!mnt.empty() && mnt[0] == '/') {
//            _base_dirs.push_back({mnt, dir});
//            return true;
//        }
//    }
//    return false;
//}

//bool TokenCache::removeMountPoint(const std::string &mount_point)
//{
//    for (auto it = _base_dirs.begin(); it != _base_dirs.end(); ++it) {
//        if (it->mount_point == mount_point) {
//            _base_dirs.erase(it);
//            return true;
//        }
//    }
//    return false;
//}

//std::vector<MountPointEntry> TokenCache::getBaseDir()
//{
//    return _base_dirs;
//}
