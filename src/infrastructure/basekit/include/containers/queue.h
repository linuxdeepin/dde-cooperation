// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_CONTAINERS_QUEUE_H
#define BASEKIT_CONTAINERS_QUEUE_H

#include <cassert>
#include <cstddef>
#include <iterator>

namespace BaseKit {

template <typename T>
class QueueIterator;
template <typename T>
class QueueConstIterator;

//! Intrusive queue container
template <typename T>
class Queue
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
    typedef QueueIterator<T> iterator;
    typedef QueueConstIterator<T> const_iterator;

    //! Queue node
    struct Node
    {
        T* next;    //!< Pointer to the next queue node

        Node() : next(nullptr) {}
    };

    Queue() noexcept : _size(0), _front(nullptr), _back(nullptr) {}
    template <class InputIterator>
    Queue(InputIterator first, InputIterator last) noexcept;
    Queue(const Queue&) noexcept = default;
    Queue(Queue&&) noexcept = default;
    ~Queue() noexcept = default;

    Queue& operator=(const Queue&) noexcept = default;
    Queue& operator=(Queue&&) noexcept = default;

    //! Check if the queue is not empty
    explicit operator bool() const noexcept { return !empty(); }

    //! Is the queue empty?
    bool empty() const noexcept { return _front == nullptr; }

    //! Get the queue size
    size_t size() const noexcept { return _size; }

    //! Get the front queue item
    T* front() noexcept { return _front; }
    const T* front() const noexcept { return _front; }
    //! Get the back queue item
    T* back() noexcept { return _back; }
    const T* back() const noexcept { return _back; }

    //! Get the begin queue iterator
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    //! Get the end queue iterator
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    //! Push a new item into the back of the queue
    /*!
        \param item - Pushed item
    */
    void push(T& item) noexcept;

    //! Pop the item from the front of the queue
    /*!
        \return The front item popped from the queue
    */
    T* pop() noexcept;

    //! Reverse the queue
    void reverse() noexcept;

    //! Clear the queue
    void clear() noexcept;

    //! Swap two instances
    void swap(Queue& queue) noexcept;
    template <typename U>
    friend void swap(Queue<U>& queue1, Queue<U>& queue2) noexcept;

private:
    size_t _size;   // Queue size
    T* _front;      // Queue front node
    T* _back;       // Queue back node
};

//! Intrusive queue iterator
/*!
    Not thread-safe.
*/
template <typename T>
class QueueIterator
{
    friend QueueConstIterator<T>;

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

    QueueIterator() noexcept : _node(nullptr) {}
    explicit QueueIterator(T* node) noexcept : _node(node) {}
    QueueIterator(const QueueIterator& it) noexcept = default;
    QueueIterator(QueueIterator&& it) noexcept = default;
    ~QueueIterator() noexcept = default;

    QueueIterator& operator=(const QueueIterator& it) noexcept = default;
    QueueIterator& operator=(QueueIterator&& it) noexcept = default;

    friend bool operator==(const QueueIterator& it1, const QueueIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const QueueIterator& it1, const QueueIterator& it2) noexcept
    { return it1._node != it2._node; }

    QueueIterator& operator++() noexcept;
    QueueIterator operator++(int) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(QueueIterator& it) noexcept;
    template <typename U>
    friend void swap(QueueIterator<U>& it1, QueueIterator<U>& it2) noexcept;

private:
    T* _node;
};

//! Intrusive queue constant iterator
/*!
    Not thread-safe.
*/
template <typename T>
class QueueConstIterator
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

    QueueConstIterator() noexcept : _node(nullptr) {}
    explicit QueueConstIterator(const T* node) noexcept : _node(node) {}
    QueueConstIterator(const QueueIterator<T>& it) noexcept : _node(it._node) {}
    QueueConstIterator(const QueueConstIterator& it) noexcept = default;
    QueueConstIterator(QueueConstIterator&& it) noexcept = default;
    ~QueueConstIterator() noexcept = default;

    QueueConstIterator& operator=(const QueueIterator<T>& it) noexcept
    { _node = it._node; return *this; }
    QueueConstIterator& operator=(const QueueConstIterator& it) noexcept = default;
    QueueConstIterator& operator=(QueueConstIterator&& it) noexcept = default;

    friend bool operator==(const QueueConstIterator& it1, const QueueConstIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const QueueConstIterator& it1, const QueueConstIterator& it2) noexcept
    { return it1._node != it2._node; }

    QueueConstIterator& operator++() noexcept;
    QueueConstIterator operator++(int) noexcept;

    const_reference operator*() const noexcept;
    const_pointer operator->() const noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(QueueConstIterator& it) noexcept;
    template <typename U>
    friend void swap(QueueConstIterator<U>& it1, QueueConstIterator<U>& it2) noexcept;

private:
    const T* _node;
};


} // namespace BaseKit

#include "queue.inl"

#endif // BASEKIT_CONTAINERS_QUEUE_H
