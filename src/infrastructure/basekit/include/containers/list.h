// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef BASEKIT_CONTAINERS_LIST_H
#define BASEKIT_CONTAINERS_LIST_H、

#include <cassert>
#include <cstddef>
#include <iterator>

namespace BaseKit {

template <typename T>
class ListIterator;
template <typename T>
class ListConstIterator;
template <typename T>
class ListReverseIterator;
template <typename T>
class ListConstReverseIterator;


//! Intrusive list container

template <typename T>
class List
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
    typedef ListIterator<T> iterator;
    typedef ListConstIterator<T> const_iterator;
    typedef ListReverseIterator<T> reverse_iterator;
    typedef ListConstReverseIterator<T> const_reverse_iterator;

    //! List node
    struct Node
    {
        T* next;    //!< Pointer to the next list node
        T* prev;    //!< Pointer to the previous list node

        Node() : next(nullptr), prev(nullptr) {}
    };

    List() noexcept : _size(0), _front(nullptr), _back(nullptr) {}
    template <class InputIterator>
    List(InputIterator first, InputIterator last) noexcept;
    List(const List&) noexcept = default;
    List(List&&) noexcept = default;
    ~List() noexcept = default;

    List& operator=(const List&) noexcept = default;
    List& operator=(List&&) noexcept = default;

    //! Check if the list is not empty
    explicit operator bool() const noexcept { return !empty(); }

    //! Is the list empty?
    bool empty() const noexcept { return _front == nullptr; }

    //! Get the list size
    size_t size() const noexcept { return _size; }

    //! Get the front list item
    T* front() noexcept { return _front; }
    const T* front() const noexcept { return _front; }
    //! Get the back list item
    T* back() noexcept { return _back; }
    const T* back() const noexcept { return _back; }

    //! Get the begin list iterator
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    //! Get the end list iterator
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    //! Get the reverse begin list iterator
    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    //! Get the reverse end list iterator
    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crend() const noexcept;

    //! Push a new item into the front of the list
    /*!
        \param item - Pushed item
    */
    void push_front(T& item) noexcept;
    //! Push a new item into the back of the list
    /*!
        \param item - Pushed item
    */
    void push_back(T& item) noexcept;
    //! Push a new item as a next to the given one
    /*!
        \param base - Base item
        \param item - Pushed item
    */
    void push_next(T& base, T& item) noexcept;
    //! Push a new item as a previous to the given one
    /*!
        \param base - Base item
        \param item - Pushed item
    */
    void push_prev(T& base, T& item) noexcept;

    //! Pop the item from the front of the list
    /*!
        \return The front item popped from the list
    */
    T* pop_front() noexcept;
    //! Pop the item from the back of the list
    /*!
        \return The back item popped from the list
    */
    T* pop_back() noexcept;
    //! Pop the given item from the list
    /*!
        \param base - Base item
        \return The given item popped from the list
    */
    T* pop_current(T& base) noexcept;
    //! Pop the next item of the given one from the list
    /*!
        \param base - Base item
        \return The next item popped from the list
    */
    T* pop_next(T& base) noexcept;
    //! Pop the previous item of the given one from the list
    /*!
        \param base - Base item
        \return The previous item popped from the list
    */
    T* pop_prev(T& base) noexcept;

    //! Reverse the list
    void reverse() noexcept;

    //! Clear the list
    void clear() noexcept;

    //! Swap two instances
    void swap(List& list) noexcept;
    template <typename U>
    friend void swap(List<U>& list1, List<U>& list2) noexcept;

private:
    size_t _size;   // List size
    T* _front;      // List front node
    T* _back;       // List back node
};

//! Intrusive list iterator
/*!
    Not thread-safe.
*/
template <typename T>
class ListIterator
{
    friend ListConstIterator<T>;

public:
    // Standard iterator type definitions
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef std::bidirectional_iterator_tag iterator_category;

    ListIterator() noexcept : _node(nullptr) {}
    explicit ListIterator(T* node) noexcept : _node(node) {}
    ListIterator(const ListIterator& it) noexcept = default;
    ListIterator(ListIterator&& it) noexcept = default;
    ~ListIterator() noexcept = default;

    ListIterator& operator=(const ListIterator& it) noexcept = default;
    ListIterator& operator=(ListIterator&& it) noexcept = default;

    friend bool operator==(const ListIterator& it1, const ListIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const ListIterator& it1, const ListIterator& it2) noexcept
    { return it1._node != it2._node; }

    ListIterator& operator++() noexcept;
    ListIterator operator++(int) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(ListIterator& it) noexcept;
    template <typename U>
    friend void swap(ListIterator<U>& it1, ListIterator<U>& it2) noexcept;

private:
    T* _node;
};

//! Intrusive list constant iterator
/*!
    Not thread-safe.
*/
template <typename T>
class ListConstIterator
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
    typedef std::bidirectional_iterator_tag iterator_category;

    ListConstIterator() noexcept : _node(nullptr) {}
    explicit ListConstIterator(const T* node) noexcept : _node(node) {}
    ListConstIterator(const ListIterator<T>& it) noexcept : _node(it._node) {}
    ListConstIterator(const ListConstIterator& it) noexcept = default;
    ListConstIterator(ListConstIterator&& it) noexcept = default;
    ~ListConstIterator() noexcept = default;

    ListConstIterator& operator=(const ListIterator<T>& it) noexcept
    { _node = it._node; return *this; }
    ListConstIterator& operator=(const ListConstIterator& it) noexcept = default;
    ListConstIterator& operator=(ListConstIterator&& it) noexcept = default;

    friend bool operator==(const ListConstIterator& it1, const ListConstIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const ListConstIterator& it1, const ListConstIterator& it2) noexcept
    { return it1._node != it2._node; }

    ListConstIterator& operator++() noexcept;
    ListConstIterator operator++(int) noexcept;

    const_reference operator*() const noexcept;
    const_pointer operator->() const noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(ListConstIterator& it) noexcept;
    template <typename U>
    friend void swap(ListConstIterator<U>& it1, ListConstIterator<U>& it2) noexcept;

private:
    const T* _node;
};

//! Intrusive list reverse iterator
/*!
    Not thread-safe.
*/
template <typename T>
class ListReverseIterator
{
    friend ListConstReverseIterator<T>;

public:
    // Standard iterator type definitions
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef std::bidirectional_iterator_tag iterator_category;

    ListReverseIterator() noexcept : _node(nullptr) {}
    explicit ListReverseIterator(T* node) noexcept : _node(node) {}
    ListReverseIterator(const ListReverseIterator& it) noexcept = default;
    ListReverseIterator(ListReverseIterator&& it) noexcept = default;
    ~ListReverseIterator() noexcept = default;

    ListReverseIterator& operator=(const ListReverseIterator& it) noexcept = default;
    ListReverseIterator& operator=(ListReverseIterator&& it) noexcept = default;

    friend bool operator==(const ListReverseIterator& it1, const ListReverseIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const ListReverseIterator& it1, const ListReverseIterator& it2) noexcept
    { return it1._node != it2._node; }

    ListReverseIterator& operator++() noexcept;
    ListReverseIterator operator++(int) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(ListReverseIterator& it) noexcept;
    template <typename U>
    friend void swap(ListReverseIterator<U>& it1, ListReverseIterator<U>& it2) noexcept;

private:
    T* _node;
};

//! Intrusive list constant reverse iterator
/*!
    Not thread-safe.
*/
template <typename T>
class ListConstReverseIterator
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
    typedef std::bidirectional_iterator_tag iterator_category;

    ListConstReverseIterator() noexcept : _node(nullptr) {}
    explicit ListConstReverseIterator(const T* node) noexcept : _node(node) {}
    ListConstReverseIterator(const ListReverseIterator<T>& it) noexcept : _node(it._node) {}
    ListConstReverseIterator(const ListConstReverseIterator& it) noexcept = default;
    ListConstReverseIterator(ListConstReverseIterator&& it) noexcept = default;
    ~ListConstReverseIterator() noexcept = default;

    ListConstReverseIterator& operator=(const ListReverseIterator<T>& it) noexcept
    { _node = it._node; return *this; }
    ListConstReverseIterator& operator=(const ListConstReverseIterator& it) noexcept = default;
    ListConstReverseIterator& operator=(ListConstReverseIterator&& it) noexcept = default;

    friend bool operator==(const ListConstReverseIterator& it1, const ListConstReverseIterator& it2) noexcept
    { return it1._node == it2._node; }
    friend bool operator!=(const ListConstReverseIterator& it1, const ListConstReverseIterator& it2) noexcept
    { return it1._node != it2._node; }

    ListConstReverseIterator& operator++() noexcept;
    ListConstReverseIterator operator++(int) noexcept;

    const_reference operator*() const noexcept;
    const_pointer operator->() const noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return _node != nullptr; }

    //! Swap two instances
    void swap(ListConstReverseIterator& it) noexcept;
    template <typename U>
    friend void swap(ListConstReverseIterator<U>& it1, ListConstReverseIterator<U>& it2) noexcept;

private:
    const T* _node;
};


} // namespace BaseKit

#include "list.inl"

#endif // BASEKIT_CONTAINERS_LIST_H
