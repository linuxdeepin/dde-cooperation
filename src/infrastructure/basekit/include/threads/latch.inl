// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline Latch::Latch(int threads) noexcept : _generation(0), _threads(threads)
{
    assert((threads > 0) && "Latch threads counter must be greater than zero!");
}

inline bool Latch::TryWaitFor(const Timespan& timespan) noexcept
{
    std::unique_lock<std::mutex> lock(_mutex);

    // Check the latch threads counter value
    if (_threads == 0)
        return true;

    // Remember the current latch generation
    int generation = _generation;

    // Wait for the next latch generation
    return _cond.wait_for(lock, timespan.chrono(), [&, this]() { return generation != _generation; });
}

inline bool Latch::TryWaitUntil(const Timestamp& timestamp) noexcept
{
    std::unique_lock<std::mutex> lock(_mutex);

    // Check the latch threads counter value
    if (_threads == 0)
        return true;

    // Remember the current latch generation
    int generation = _generation;

    // Wait for the next latch generation
    return _cond.wait_until(lock, timestamp.chrono(), [&, this]() { return generation != _generation; });
}

} // namespace BaseKit
