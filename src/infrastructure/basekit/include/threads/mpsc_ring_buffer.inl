// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

inline MPSCRingBuffer::MPSCRingBuffer(size_t capacity, size_t concurrency) : _capacity(capacity - 1), _concurrency(concurrency), _consumer(0)
{
    // Initialize producers' ring buffer
    for (size_t i = 0; i < concurrency; ++i)
        _producers.push_back(std::make_shared<Producer>(capacity));
}

inline size_t MPSCRingBuffer::size() const noexcept
{
    size_t size = 0;
    for (const auto& producer : _producers)
        size += producer->buffer.size();
    return size;
}

inline bool MPSCRingBuffer::Enqueue(const void* data, size_t size)
{
    // Get producer index for the current thread based on RDTS value
    size_t index = Timestamp::rdts() % _concurrency;

    // Lock the chosen producer using its spin-lock
    Locker<SpinLock> lock(_producers[index]->lock);

    // Enqueue the item into the producer's ring buffer
    return _producers[index]->buffer.Enqueue(data, size);
}

inline bool MPSCRingBuffer::Dequeue(void* data, size_t& size)
{
    // Try to dequeue one item from the one of producer's ring buffers
    for (size_t i = 0; i < _concurrency; ++i)
    {
        size_t temp = size;
        if (_producers[_consumer++ % _concurrency]->buffer.Dequeue(data, temp))
        {
            size = temp;
            return true;
        }
    }

    size = 0;
    return false;
}

} // namespace BaseKit
