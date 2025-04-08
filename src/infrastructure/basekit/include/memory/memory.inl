// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#include <cstdint>

namespace BaseKit {

inline bool Memory::IsValidAlignment(size_t alignment) noexcept
{
    return ((alignment > 0) && ((alignment & (alignment - 1)) == 0));
}

template <typename T>
inline bool Memory::IsAligned(const T* address, size_t alignment) noexcept
{
    assert((address != nullptr) && "Address must be valid!");
    assert(IsValidAlignment(alignment) && "Alignment must be valid!");

    uintptr_t ptr = (uintptr_t)address;
    return (ptr & (alignment - 1)) == 0;
}

template <typename T>
inline T* Memory::Align(const T* address, size_t alignment, bool upwards) noexcept
{
    assert((address != nullptr) && "Address must be valid!");
    assert(IsValidAlignment(alignment) && "Alignment must be valid!");

    uintptr_t ptr = (uintptr_t)address;

    if (upwards)
        return (T*)((ptr + (alignment - 1)) & -((int)alignment));
    else
        return (T*)(ptr & -((int)alignment));
}

} // namespace BaseKit
