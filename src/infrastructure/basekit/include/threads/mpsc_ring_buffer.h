// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_THREADS_MPSC_RING_BUFFER_H
#define BASEKIT_THREADS_MPSC_RING_BUFFER_H

#include "threads/spin_lock.h"
#include "threads/spsc_ring_buffer.h"
#include "time/timestamp.h"

#include <cstdio>
#include <memory>
#include <thread>
#include <vector>

namespace BaseKit {

//! Multiple producers / single consumer wait-free ring buffer
/*!
    Multiple producers / single consumer wait-free ring buffer use only atomic operations to provide thread-safe
    enqueue and dequeue operations. This data structure consist of several SPSC ring buffers which count is
    provided as a hardware concurrency in the constructor. All of them are randomly accessed with a RDTS
    distribution index. All the items available in sesequential or batch mode. All ring buffer sizes are
    limited to the capacity provided in the constructor.

    FIFO order is not guaranteed!

    Thread-safe.
*/
class MPSCRingBuffer
{
public:
    //! Default class constructor
    /*!
        \param capacity - Ring buffer capacity (must be a power of two)
        \param concurrency - Hardware concurrency (default is std::thread::hardware_concurrency)
    */
    explicit MPSCRingBuffer(size_t capacity, size_t concurrency = std::thread::hardware_concurrency());
    MPSCRingBuffer(const MPSCRingBuffer&) = delete;
    MPSCRingBuffer(MPSCRingBuffer&&) = delete;
    ~MPSCRingBuffer() = default;

    MPSCRingBuffer& operator=(const MPSCRingBuffer&) = delete;
    MPSCRingBuffer& operator=(MPSCRingBuffer&&) = delete;

    //! Check if the buffer is not empty
    explicit operator bool() const noexcept { return !empty(); }

    //! Is ring buffer empty?
    bool empty() const noexcept { return (size() == 0); }
    //! Get ring buffer capacity
    size_t capacity() const noexcept { return _capacity; }
    //! Get ring buffer concurrency
    size_t concurrency() const noexcept { return _concurrency; }
    //! Get ring buffer size
    size_t size() const noexcept;

    //! Enqueue a data into the ring buffer (single producer thread method)
    /*!
        The data will be copied into the ring buffer using 'memcpy()' function.
        Data size should not be greater than ring buffer capacity!

        Will not block.

        \param data - Data buffer to enqueue
        \param size - Data buffer size
        \return 'true' if the data was successfully enqueue, 'false' if the ring buffer is full
    */
    bool Enqueue(const void* data, size_t size);

    //! Dequeue a data from the ring buffer (single consumer thread method)
    /*!
        The data will be copied from the ring buffer using 'memcpy()' function.

        Will not block.

        \param data - Data buffer to dequeue
        \param size - Data buffer size
        \return 'true' if the data was successfully dequeue, 'false' if the ring buffer is empty
    */
    bool Dequeue(void* data, size_t& size);

private:
    struct Producer
    {
        SpinLock lock;
        SPSCRingBuffer buffer;

        Producer(size_t capacity) : buffer(capacity) {}
    };

    size_t _capacity;
    size_t _concurrency;
    std::vector<std::shared_ptr<Producer>> _producers;
    size_t _consumer;
};

/*! \example threads_mpsc_ring_buffer.cpp Multiple producers / single consumer wait-free ring buffer example */

} // namespace BaseKit

#include "mpsc_ring_buffer.inl"

#endif // BASEKIT_THREADS_MPSC_RING_BUFFER_H
