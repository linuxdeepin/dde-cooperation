// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline bool FileCache::empty() const
{
    std::shared_lock<std::shared_mutex> locker(_lock);
    return _entries_by_key.empty();
}

inline size_t FileCache::size() const
{
    std::shared_lock<std::shared_mutex> locker(_lock);
    return _entries_by_key.size();
}

inline void swap(FileCache& cache1, FileCache& cache2) noexcept
{
    cache1.swap(cache2);
}

} // namespace BaseKit
