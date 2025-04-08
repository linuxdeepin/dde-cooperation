// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cache/filecache.h"

namespace BaseKit {

bool FileCache::emplace(std::string&& key, std::string&& value, const Timespan& timeout)
{
    std::unique_lock<std::shared_mutex> locker(_lock);

    // Try to find and remove the previous key
    remove_internal(key);

    // Update the cache entry
    if (timeout.total() > 0)
    {
        Timestamp current = UtcTimestamp();
        _timestamp = (current <= _timestamp) ? _timestamp + 1 : current;
        _entries_by_key.insert(std::make_pair(key, MemCacheEntry(std::move(value), _timestamp, timeout)));
        _entries_by_timestamp.insert(std::make_pair(_timestamp, key));
    }
    else
        _entries_by_key.emplace(std::make_pair(std::move(key), MemCacheEntry(std::move(value))));

    return true;
}

bool FileCache::insert(const std::string& key, const std::string& value, const Timespan& timeout)
{
    std::unique_lock<std::shared_mutex> locker(_lock);

    // Try to find and remove the previous key
    remove_internal(key);

    // Update the cache entry
    if (timeout.total() > 0)
    {
        Timestamp current = UtcTimestamp();
        _timestamp = (current <= _timestamp) ? _timestamp + 1 : current;
        _entries_by_key.insert(std::make_pair(key, MemCacheEntry(value, _timestamp, timeout)));
        _entries_by_timestamp.insert(std::make_pair(_timestamp, key));
    }
    else
        _entries_by_key.insert(std::make_pair(key, MemCacheEntry(value)));

    return true;
}

std::pair<bool, std::string_view> FileCache::find(const std::string& key)
{
    std::shared_lock<std::shared_mutex> locker(_lock);

    // Try to find the given key
    auto it = _entries_by_key.find(key);
    if (it == _entries_by_key.end())
        return std::make_pair(false, std::string_view());

    return std::make_pair(true, std::string_view(it->second.value));
}

std::pair<bool, std::string_view> FileCache::find(const std::string& key, Timestamp& timeout)
{
    std::shared_lock<std::shared_mutex> locker(_lock);

    // Try to find the given key
    auto it = _entries_by_key.find(key);
    if (it == _entries_by_key.end())
        return std::make_pair(false, std::string_view());

    timeout = it->second.timestamp + it->second.timespan;
    return std::make_pair(true, std::string_view(it->second.value));
}

bool FileCache::remove(const std::string& key)
{
    std::unique_lock<std::shared_mutex> locker(_lock);

    return remove_internal(key);
}

bool FileCache::remove_internal(const std::string& key)
{
    // Try to find the given key
    auto it = _entries_by_key.find(key);
    if (it == _entries_by_key.end())
        return false;

    // Try to erase cache entry by timestamp
    if (it->second.timestamp.total() > 0)
        _entries_by_timestamp.erase(it->second.timestamp);

    // Erase cache entry
    _entries_by_key.erase(it);

    return true;
}

bool FileCache::insert_path(const BaseKit::Path& path, const std::string& prefix, const Timespan& timeout, const InsertHandler& handler)
{
    // Try to find and remove the previous path
    remove_path_internal(path);

    // Insert the cache path
    if (!insert_path_internal(path, prefix, timeout, handler))
        return false;

    std::unique_lock<std::shared_mutex> locker(_lock);

    // Update the cache path
    if (timeout.total() > 0)
    {
        Timestamp current = UtcTimestamp();
        _timestamp = (current <= _timestamp) ? _timestamp + 1 : current;
        _paths_by_key.insert(std::make_pair(path, FileCacheEntry(prefix, handler, _timestamp, timeout)));
        _paths_by_timestamp.insert(std::make_pair(_timestamp, path));
    }
    else
        _paths_by_key.insert(std::make_pair(path, FileCacheEntry(prefix, handler)));

    return true;
}

bool FileCache::insert_path_internal(const BaseKit::Path& path, const std::string& prefix, const Timespan& timeout, const InsertHandler& handler)
{
    try
    {
        const std::string key_prefix = (prefix.empty() || (prefix == "/")) ? "/" : (prefix + "/");

        // Iterate through all directory entries
        for (const auto& item : BaseKit::Directory(path))
        {
            const BaseKit::Path entry = item.IsSymlink() ? Symlink(item).target() : item;
            const std::string key = key_prefix + BaseKit::Encoding::URLDecode(item.filename().string());

            if (entry.IsDirectory())
            {
                // Recursively insert sub-directory
                if (!insert_path_internal(entry, key, timeout, handler))
                    return false;
            }
            else
            {
                try
                {
                    // Load the cache file content
                    auto content = BaseKit::File::ReadAllBytes(entry);
                    std::string value(content.begin(), content.end());
                    if (!handler(*this, key, value, timeout))
                        return false;
                }
                catch (const BaseKit::FileSystemException&) { return false; }
            }
        }

        return true;
    }
    catch (const BaseKit::FileSystemException&) { return false; }
}

bool FileCache::find_path(const BaseKit::Path& path)
{
    std::shared_lock<std::shared_mutex> locker(_lock);

    // Try to find the given path
    auto it = _paths_by_key.find(path);
    if (it == _paths_by_key.end())
        return false;

    return true;
}

bool FileCache::find_path(const BaseKit::Path& path, Timestamp& timeout)
{
    std::shared_lock<std::shared_mutex> locker(_lock);

    // Try to find the given path
    auto it = _paths_by_key.find(path);
    if (it == _paths_by_key.end())
        return false;

    timeout = it->second.timestamp + it->second.timespan;
    return true;
}

bool FileCache::remove_path(const BaseKit::Path& path)
{
    return remove_path_internal(path);
}

bool FileCache::remove_path_internal(const BaseKit::Path& path)
{
    std::unique_lock<std::shared_mutex> locker(_lock);

    // Try to find the given path
    auto it = _paths_by_key.find(path);
    if (it == _paths_by_key.end())
        return false;

    // Try to erase cache path by timestamp
    if (it->second.timestamp.total() > 0)
        _paths_by_timestamp.erase(it->second.timestamp);

    // Erase cache path
    _paths_by_key.erase(it);

    return true;
}

void FileCache::clear()
{
    std::unique_lock<std::shared_mutex> locker(_lock);

    // Clear all cache entries
    _entries_by_key.clear();
    _entries_by_timestamp.clear();
    _paths_by_key.clear();
    _paths_by_timestamp.clear();
}

void FileCache::watchdog(const UtcTimestamp& utc)
{
    std::unique_lock<std::shared_mutex> locker(_lock);

    // Watchdog for cache entries
    auto it_entry_by_timestamp = _entries_by_timestamp.begin();
    while (it_entry_by_timestamp != _entries_by_timestamp.end())
    {
        // Check for the cache entry timeout
        auto it_entry_by_key = _entries_by_key.find(it_entry_by_timestamp->second);
        if ((it_entry_by_key->second.timestamp + it_entry_by_key->second.timespan) <= utc)
        {
            // Erase the cache entry with timeout
            _entries_by_key.erase(it_entry_by_key);
            _entries_by_timestamp.erase(it_entry_by_timestamp);
            it_entry_by_timestamp = _entries_by_timestamp.begin();
            continue;
        }
        else
            break;
    }

    // Watchdog for cache paths
    auto it_path_by_timestamp = _paths_by_timestamp.begin();
    while (it_path_by_timestamp != _paths_by_timestamp.end())
    {
        // Check for the cache entry timeout
        auto& it_path_by_key = _paths_by_key[it_path_by_timestamp->second];
        if ((it_path_by_key.timestamp + it_path_by_key.timespan) <= utc)
        {
            // Update the cache path with timeout
            auto path = it_path_by_timestamp->second;
            auto prefix = it_path_by_key.prefix;
            auto timespan = it_path_by_key.timespan;
            auto handler = it_path_by_key.handler;
            _paths_by_timestamp.erase(it_path_by_timestamp);
            locker.unlock();
            insert_path(path, prefix, timespan, handler);
            locker.lock();
            it_path_by_timestamp = _paths_by_timestamp.begin();
            continue;
        }
        else
            break;
    }
}

void FileCache::swap(FileCache& cache) noexcept
{
    std::unique_lock<std::shared_mutex> locker1(_lock);
    std::unique_lock<std::shared_mutex> locker2(cache._lock);

    using std::swap;
    swap(_timestamp, cache._timestamp);
    swap(_entries_by_key, cache._entries_by_key);
    swap(_entries_by_timestamp, cache._entries_by_timestamp);
    swap(_paths_by_key, cache._paths_by_key);
    swap(_paths_by_timestamp, cache._paths_by_timestamp);
}

} // namespace BaseKit
