// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_FUNCTION_H
#define BASEKIT_FUNCTION_H

#include <functional>
#include <memory>

namespace BaseKit {

//! Allocation free function stub
template <class, size_t Capacity = 1024>
class Function;

//! Allocation free function
/*!
    \note This class is not intended to be used directly, use the Function macro instead.
    \note The function must be copyable and movable.
*/
template <class R, class... Args, size_t Capacity>
class Function<R(Args...), Capacity>
{
public:
    Function() noexcept;
    Function(std::nullptr_t) noexcept;
    Function(const Function& function) noexcept;
    Function(Function&& function) noexcept;
    template <class TFunction>
    Function(TFunction&& function) noexcept;
    ~Function() noexcept;

    Function& operator=(std::nullptr_t) noexcept;
    Function& operator=(const Function& function) noexcept;
    Function& operator=(Function&& function) noexcept;
    template <typename TFunction>
    Function& operator=(TFunction&& function) noexcept;
    template <typename TFunction>
    Function& operator=(std::reference_wrapper<TFunction> function) noexcept;

    //! Check if the function is valid
    explicit operator bool() const noexcept { return (_manager != nullptr); }

    //! Invoke the function
    R operator()(Args... args);

    //! Swap two instances
    void swap(Function& function) noexcept;
    template <class UR, class... UArgs, size_t UCapacity>
    friend void swap(Function<UR(UArgs...), UCapacity>& function1, Function<UR(UArgs...), UCapacity>& function2) noexcept;

private:
    enum class Operation { Clone, Destroy };

    using Invoker = R (*)(void*, Args&&...);
    using Manager = void (*)(void*, void*, Operation);

    static const size_t StorageSize = Capacity - sizeof(Invoker) - sizeof(Manager);
    static const size_t StorageAlign = 8;

    alignas(StorageAlign) std::byte _data[StorageSize];
    Invoker _invoker;
    Manager _manager;

    template <typename TFunction>
    static R Invoke(void* data, Args&&... args) noexcept;

    template <typename TFunction>
    static void Manage(void* dst, void* src, Operation op) noexcept;
};


} // namespace BaseKit

#include "function.inl"

#endif // BASEKIT_FUNCTION_H
