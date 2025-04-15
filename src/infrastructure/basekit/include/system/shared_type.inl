// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

template <typename T>
inline SharedType<T>::SharedType(const std::string& name) : _shared(name, sizeof(T))
{
    // Check for the owner flag
    if (_shared.owner())
    {
        // Call in place constructor
        new (_shared.ptr()) T();
    }
}

} // namespace BaseKit
