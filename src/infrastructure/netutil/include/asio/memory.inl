// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace NetUtil {
namespace Asio {

inline void* HandlerStorage::allocate(size_t size)
{
    // Check if the storage is not already used and has enough capacity
    if (!_in_use && (size < sizeof(_storage)))
    {
        _in_use = true;
        return &_storage;
    }

    // Otherwise allocate memory in the heap
    return ::operator new(size);
}

inline void HandlerStorage::deallocate(void* ptr)
{
    // Free storage if memory block was allocated from it
    if (ptr == &_storage)
    {
        _in_use = false;
        return;
    }

    // Otherwise free memory in the heap
    ::operator delete(ptr);
}

template <typename THandler>
inline AllocateHandler<THandler> make_alloc_handler(HandlerStorage& storage, THandler handler)
{
    return AllocateHandler<THandler>(storage, handler);
}

} // namespace Asio
} // namespace NetUtil
