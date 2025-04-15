// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later


#ifndef BASEKIT_CONTAINERS_STACK_H
#define BASEKIT_CONTAINERS_STACK_H

#include <cassert>
#include <cstddef>
#include <iterator>

namespace BaseKit {

template <typename T>
class StackIterator;
template <typename T>
class StackConstIterator;

//! Intrusive stack container
template <typename T>
class Stack
{
public:
    // Standard container type definitions
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef StackIterator<T> iterator;
    typedef StackConstIterator<T> const_iterator;

    //! Stack node
    struct Node
    {
        T* next;    //!< Pointer to the next stack node

        Node() : next(nullptr) {}
    };

    Stack() noexcept : _size(0), _top(nullptr) {}
    template <class InputIterator>
    Stack(InputIterator first, InputIterator last) noexcept;
    Stack(const Stack&) noexcept = default;
    Stack(Stack&&) noexcept = default;
    ~Stack() noexcept = default;

    Stack& operator=(const Stack&) noexcept = default;
    Stack& operator=(Stack&&) noexcept = default;

    //! Check if the stack is not empty
    explicit operator bool() const noexcept { return !empty(); }

    //! Is the stack empty?
    bool empty() const noexcept { return _top == nullptr; }

    //! Get the stack size
    size_t size() const noexcept { return _size; }

    //! Get the top stack item
    T* top() noexcept { return _top; }
    const T* top() const noexcept { return _top; }

    //! Get the begin stack iterator
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    //! Get the end stack iterator
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    //! Push a new item into the top of the stack
    /*!
        \param item - Pushed item
    */
    void push(T& item) noexcept;

    //! Pop the item from the top of the stack
    /*!
        \return The top item popped from the stack
    */
    T* pop() noexcept;

    //! Reverse the stack
    void reverse() noexcept;

    //! Clear the stack
    void clear() noexcept;

    //! Swap two instances
    void swap(Stack& stack) noexcept;
    template <typename U>
    friend void swap(Stack<U>& stack1, Stack<U>& stack2) noexcept;

private:
    size_t _size;   // Stack size
    T* _top;        // Stack top node
};

//! Intrusive stack iterator
/*!
    Not thread-safe.
*/
template <typename T>
class StackIterator
{
    friend StackConstIterator<T>;

public:
    // Standard iterator type definitions
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef std::forward_iterator_tag iterator_category;

    StackIterator() noexcept : _node(nullptr) {}
    explicit StackIterator(T* node) noexcept : _node(node) {}
    StackIterator(const StackIterator& it) noexcept = default;
    StackIterator(StackIterator&& it) noexcept = default;
    ~StackIterator() noexcept = default;

    StackIterator& operator=(const StackIterator& it) noexcept = default;
    StackIterator& operator=(StackIterator&& it) noexcept = default;

    friend bool operator==(const StackIterator& it1, const StackIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const StackIterator& it1, const StackIterator& it2) noexcept
    { return it1._node != it2._node; }

    StackIterator& operator++() noexcept;
    StackIterator operator++(int) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(StackIterator& it) noexcept;
    template <typename U>
    friend void swap(StackIterator<U>& it1, StackIterator<U>& it2) noexcept;

private:
    T* _node;
};

//! Intrusive stack constant iterator
/*!
    Not thread-safe.
*/
template <typename T>
class StackConstIterator
{
public:
    // Standard iterator type definitions
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef std::forward_iterator_tag iterator_category;

    StackConstIterator() noexcept : _node(nullptr) {}
    explicit StackConstIterator(const T* node) noexcept : _node(node) {}
    StackConstIterator(const StackIterator<T>& it) noexcept : _node(it._node) {}
    StackConstIterator(const StackConstIterator& it) noexcept = default;
    StackConstIterator(StackConstIterator&& it) noexcept = default;
    ~StackConstIterator() noexcept = default;

    StackConstIterator& operator=(const StackIterator<T>& it) noexcept
    { _node = it._node; return *this; }
    StackConstIterator& operator=(const StackConstIterator& it) noexcept = default;
    StackConstIterator& operator=(StackConstIterator&& it) noexcept = default;

    friend bool operator==(const StackConstIterator& it1, const StackConstIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const StackConstIterator& it1, const StackConstIterator& it2) noexcept
    { return it1._node != it2._node; }

    StackConstIterator& operator++() noexcept;
    StackConstIterator operator++(int) noexcept;

    const_reference operator*() const noexcept;
    const_pointer operator->() const noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(StackConstIterator& it) noexcept;
    template <typename U>
    friend void swap(StackConstIterator<U>& it1, StackConstIterator<U>& it2) noexcept;

private:
    const T* _node;
};


} // namespace BaseKit

#include "stack.inl"

#endif // BASEKIT_CONTAINERS_STACK_H
