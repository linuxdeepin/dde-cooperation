// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_UTILITY_RESOURCE_H
#define BASEKIT_UTILITY_RESOURCE_H

#include <memory>

namespace BaseKit {

//! Resource smart cleaner pattern
/*!
    Resource smart cleaner pattern allows to create unique smart pointer with
    a given resource and cleaner delegate which is used to automatic resource
    clean when it goes out of scope.

    Thread-safe.

    Example:
    \code{.cpp}
    int test()
    {
        // Create a file resource
        auto file = BaseKit::resource(fopen("test", "rb"), [](FILE* file) { fclose(file); });

        // Work with the file resource
        int result = fgetc(file.get());

        // File resource will be cleaned automatically when we go out of scope
        return result;
    }
    \endcode

    \param handle - Resource handle
    \param cleaner - Cleaner function
*/
template <typename T, typename TCleaner>
auto resource(T handle, TCleaner cleaner)
{
    return std::unique_ptr<typename std::remove_pointer<T>::type, TCleaner>(handle, cleaner);
}

//! Resource smart cleaner pattern (void* specialization)
/*!
    \param handle - Resource handle
    \param cleaner - Cleaner function
*/
template <typename TCleaner>
auto resource(void* handle, TCleaner cleaner)
{
    return std::unique_ptr<void, TCleaner>(handle, cleaner);
}

//! Resource smart cleaner pattern for empty resource handle
/*!
    \param cleaner - Cleaner function
*/
template <typename TCleaner>
auto resource(TCleaner cleaner)
{
    return std::unique_ptr<void, TCleaner>(&cleaner, cleaner);
}

} // namespace BaseKit

#endif // BASEKIT_UTILITY_RESOURCE_H
