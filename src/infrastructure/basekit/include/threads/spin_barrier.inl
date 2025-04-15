// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline SpinBarrier::SpinBarrier(int threads) noexcept : _counter(threads), _generation(0), _threads(threads)
{
    assert((threads > 0) && "Count of barrier threads must be greater than zero!");
}

inline bool SpinBarrier::Wait() noexcept
{
    // Remember the current barrier generation
    int generation = _generation;

    // Decrease the count of waiting threads
    if (--_counter == 0)
    {
        // Increase the current barrier generation
        ++_generation;

        // Reset waiting threads counter
        _counter = _threads;

        // Notify the last thread that reached the barrier
        return true;
    }
    else
    {
        // Spin-wait for the next barrier generation
        while ((generation == _generation) || (_counter == 0));

        // Notify each of remaining threads
        return false;
    }
}

} // namespace BaseKit
