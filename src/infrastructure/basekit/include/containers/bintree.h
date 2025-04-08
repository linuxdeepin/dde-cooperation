// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_CONTAINERS_BINTREE_H
#define BASEKIT_CONTAINERS_BINTREE_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <utility>

namespace BaseKit {

template <class TContainer, typename T>
class BinTreeIterator;
template <class TContainer, typename T>
class BinTreeConstIterator;
template <class TContainer, typename T>
class BinTreeReverseIterator;
template <class TContainer, typename T>
class BinTreeConstReverseIterator;

template <typename T, typename TCompare = std::less<T>>
class BinTree
{
public:
    // Standard container type definitions
    typedef T value_type;
    typedef TCompare value_compare;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;
    typedef BinTreeIterator<BinTree<T, TCompare>, T> iterator;
    typedef BinTreeConstIterator<BinTree<T, TCompare>, T> const_iterator;
    typedef BinTreeReverseIterator<BinTree<T, TCompare>, T> reverse_iterator;
    typedef BinTreeConstReverseIterator<BinTree<T, TCompare>, T> const_reverse_iterator;

    struct Node
    {
        T* parent;
        T* left;
        T* right;

        Node() : parent(nullptr), left(nullptr), right(nullptr) {}
    };

    explicit BinTree(const TCompare& compare = TCompare()) noexcept : _compare(compare), _size(0), _root(nullptr) {}
    template <class InputIterator>
    BinTree(InputIterator first, InputIterator last, const TCompare& compare = TCompare()) noexcept;
    BinTree(const BinTree&) noexcept = default;
    BinTree(BinTree&&) noexcept = default;
    ~BinTree() noexcept = default;

    BinTree& operator=(const BinTree&) noexcept = default;
    BinTree& operator=(BinTree&&) noexcept = default;

    //! Check if the binary tree is not empty
    explicit operator bool() const noexcept { return !empty(); }

    //! Is the binary tree empty?
    bool empty() const noexcept { return _root == nullptr; }

    //! Get the binary tree size
    size_t size() const noexcept { return _size; }

    //! Get the root binary tree item
    T* root() noexcept { return _root; }
    const T* root() const noexcept { return _root; }
    //! Get the lowest binary tree item
    T* lowest() noexcept;
    const T* lowest() const noexcept;
    //! Get the highest binary tree item
    T* highest() noexcept;
    const T* highest() const noexcept;

    //! Compare two items: if the first item is less than the second one?
    bool compare(const T& item1, const T& item2) const noexcept { return _compare(item1, item2); }

    //! Get the begin binary tree iterator
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    //! Get the end binary tree iterator
    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    //! Get the reverse begin binary tree iterator
    reverse_iterator rbegin() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator crbegin() const noexcept;
    //! Get the reverse end binary tree iterator
    reverse_iterator rend() noexcept;
    const_reverse_iterator rend() const noexcept;
    const_reverse_iterator crend() const noexcept;

    //! Find the iterator which points to the first equal item in the binary tree or return end iterator
    iterator find(const T& item) noexcept;
    const_iterator find(const T& item) const noexcept;

    //! Find the iterator which points to the first item that not less than the given item in the binary tree or return end iterator
    iterator lower_bound(const T& item) noexcept;
    const_iterator lower_bound(const T& item) const noexcept;
    //! Find the iterator which points to the first item that greater than the given item in the binary tree or return end iterator
    iterator upper_bound(const T& item) noexcept;
    const_iterator upper_bound(const T& item) const noexcept;

    //! Insert a new item into the binary tree
    std::pair<iterator, bool> insert(T& item) noexcept;
    //! Insert a new item into the binary tree with a position hint
    std::pair<iterator, bool> insert(const const_iterator& position, T& item) noexcept;

    //! Erase the given item from the binary tree
    T* erase(const T& item) noexcept;
    //! Erase the given item from the binary tree
    iterator erase(const iterator& it) noexcept;

    //! Clear the binary tree
    void clear() noexcept;

    //! Swap two instances
    void swap(BinTree& bintree) noexcept;
    template <typename U, typename UCompare>
    friend void swap(BinTree<U, UCompare>& bintree1, BinTree<U, UCompare>& bintree2) noexcept;

private:
    TCompare _compare;  // Binary tree node comparator
    size_t _size;       // Binary tree size
    T* _root;           // Binary tree root node

    const T* InternalLowest() const noexcept;
    const T* InternalHighest() const noexcept;
    const T* InternalFind(const T& item) const noexcept;
    const T* InternalLowerBound(const T& item) const noexcept;
    const T* InternalUpperBound(const T& item) const noexcept;
};

//! Intrusive binary tree iterator
template <class TContainer, typename T>
class BinTreeIterator
{
    friend BinTreeConstIterator<TContainer, T>;

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

    BinTreeIterator() noexcept : _container(nullptr), _node(nullptr) {}
    explicit BinTreeIterator(TContainer* container, T* node) noexcept : _container(container), _node(node) {}
    BinTreeIterator(const BinTreeIterator& it) noexcept = default;
    BinTreeIterator(BinTreeIterator&& it) noexcept = default;
    ~BinTreeIterator() noexcept = default;

    BinTreeIterator& operator=(const BinTreeIterator& it) noexcept = default;
    BinTreeIterator& operator=(BinTreeIterator&& it) noexcept = default;

    friend bool operator==(const BinTreeIterator& it1, const BinTreeIterator& it2) noexcept
    { return (it1._container == it2._container) && (it1._node == it2._node); }
    friend bool operator!=(const BinTreeIterator& it1, const BinTreeIterator& it2) noexcept
    { return (it1._container != it2._container) || (it1._node != it2._node); }

    BinTreeIterator& operator++() noexcept;
    BinTreeIterator operator++(int) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return (_container != nullptr) && (_node != nullptr); }

    //! Compare two items: if the first item is less than the second one?
    bool compare(const T& item1, const T& item2) const noexcept { return (_container != nullptr) ? _container->compare(item1, item2) : false; }

    //! Swap two instances
    void swap(BinTreeIterator& it) noexcept;
    template <class UContainer, typename U>
    friend void swap(BinTreeIterator<UContainer, U>& it1, BinTreeIterator<UContainer, U>& it2) noexcept;

private:
    TContainer* _container;
    T* _node;
};

//! Intrusive binary tree constant iterator
template <class TContainer, typename T>
class BinTreeConstIterator
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

    BinTreeConstIterator() noexcept : _container(nullptr), _node(nullptr) {}
    explicit BinTreeConstIterator(const TContainer* container, const T* node) noexcept : _container(container), _node(node) {}
    BinTreeConstIterator(const BinTreeIterator<TContainer, T>& it) noexcept  : _container(it._container), _node(it._node) {}
    BinTreeConstIterator(const BinTreeConstIterator& it) noexcept = default;
    BinTreeConstIterator(BinTreeConstIterator&& it) noexcept = default;
    ~BinTreeConstIterator() noexcept = default;

    BinTreeConstIterator& operator=(const BinTreeIterator<TContainer, T>& it) noexcept
    { _container = it._container; _node = it._node; return *this; }
    BinTreeConstIterator& operator=(const BinTreeConstIterator& it) noexcept = default;
    BinTreeConstIterator& operator=(BinTreeConstIterator&& it) noexcept = default;

    friend bool operator==(const BinTreeConstIterator& it1, const BinTreeConstIterator& it2) noexcept
    { return (it1._container == it2._container) && (it1._node == it2._node); }
    friend bool operator!=(const BinTreeConstIterator& it1, const BinTreeConstIterator& it2) noexcept
    { return (it1._container != it2._container) || (it1._node != it2._node); }

    BinTreeConstIterator& operator++() noexcept;
    BinTreeConstIterator operator++(int) noexcept;

    const_reference operator*() const noexcept;
    const_pointer operator->() const noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return (_container != nullptr) && (_node != nullptr); }

    //! Compare two items: if the first item is less than the second one?
    bool compare(const T& item1, const T& item2) const noexcept { return (_container != nullptr) ? _container->compare(item1, item2) : false; }

    //! Swap two instances
    void swap(BinTreeConstIterator& it) noexcept;
    template <class UContainer, typename U>
    friend void swap(BinTreeConstIterator<UContainer, U>& it1, BinTreeConstIterator<UContainer, U>& it2) noexcept;

private:
    const TContainer* _container;
    const T* _node;
};

//! Intrusive binary tree reverse iterator
template <class TContainer, typename T>
class BinTreeReverseIterator
{
    friend BinTreeConstReverseIterator<TContainer, T>;

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

    BinTreeReverseIterator() noexcept : _container(nullptr), _node(nullptr) {}
    explicit BinTreeReverseIterator(TContainer* container, T* node) noexcept : _container(container), _node(node) {}
    BinTreeReverseIterator(const BinTreeReverseIterator& it) noexcept = default;
    BinTreeReverseIterator(BinTreeReverseIterator&& it) noexcept = default;
    ~BinTreeReverseIterator() noexcept = default;

    BinTreeReverseIterator& operator=(const BinTreeReverseIterator& it) noexcept = default;
    BinTreeReverseIterator& operator=(BinTreeReverseIterator&& it) noexcept = default;

    friend bool operator==(const BinTreeReverseIterator& it1, const BinTreeReverseIterator& it2) noexcept
    { return (it1._container == it2._container) && (it1._node == it2._node); }
    friend bool operator!=(const BinTreeReverseIterator& it1, const BinTreeReverseIterator& it2) noexcept
    { return (it1._container != it2._container) || (it1._node != it2._node); }

    BinTreeReverseIterator& operator++() noexcept;
    BinTreeReverseIterator operator++(int) noexcept;

    reference operator*() noexcept;
    pointer operator->() noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return (_container != nullptr) && (_node != nullptr); }

    //! Compare two items: if the first item is less than the second one?
    bool compare(const T& item1, const T& item2) const noexcept { return (_container != nullptr) ? _container->compare(item1, item2) : false; }

    //! Swap two instances
    void swap(BinTreeReverseIterator& it) noexcept;
    template <class UContainer, typename U>
    friend void swap(BinTreeReverseIterator<UContainer, U>& it1, BinTreeReverseIterator<UContainer, U>& it2) noexcept;

private:
    TContainer* _container;
    T* _node;
};

//! Intrusive binary tree constant reverse iterator
template <class TContainer, typename T>
class BinTreeConstReverseIterator
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

    BinTreeConstReverseIterator() noexcept : _container(nullptr), _node(nullptr) {}
    explicit BinTreeConstReverseIterator(const TContainer* container, const T* node) noexcept : _container(container), _node(node) {}
    BinTreeConstReverseIterator(const BinTreeReverseIterator<TContainer, T>& it) noexcept : _container(it._container), _node(it._node) {}
    BinTreeConstReverseIterator(const BinTreeConstReverseIterator& it) noexcept = default;
    BinTreeConstReverseIterator(BinTreeConstReverseIterator&& it) noexcept = default;
    ~BinTreeConstReverseIterator() noexcept = default;

    BinTreeConstReverseIterator& operator=(const BinTreeReverseIterator<TContainer, T>& it) noexcept
    { _container = it._container; _node = it._node; return *this; }
    BinTreeConstReverseIterator& operator=(const BinTreeConstReverseIterator& it) noexcept = default;
    BinTreeConstReverseIterator& operator=(BinTreeConstReverseIterator&& it) noexcept = default;

    friend bool operator==(const BinTreeConstReverseIterator& it1, const BinTreeConstReverseIterator& it2) noexcept
    { return (it1._container == it2._container) && (it1._node == it2._node); }
    friend bool operator!=(const BinTreeConstReverseIterator& it1, const BinTreeConstReverseIterator& it2) noexcept
    { return (it1._container != it2._container) || (it1._node != it2._node); }

    BinTreeConstReverseIterator& operator++() noexcept;
    BinTreeConstReverseIterator operator++(int) noexcept;

    const_reference operator*() const noexcept;
    const_pointer operator->() const noexcept;

    //! Check if the iterator is valid
    explicit operator bool() const noexcept { return (_container != nullptr) && (_node != nullptr); }

    //! Compare two items: if the first item is less than the second one?
    bool compare(const T& item1, const T& item2) const noexcept { return (_container != nullptr) ? _container->compare(item1, item2) : false; }

    //! Swap two instances
    void swap(BinTreeConstReverseIterator& it) noexcept;
    template <class UContainer, typename U>
    friend void swap(BinTreeConstReverseIterator<UContainer, U>& it1, BinTreeConstReverseIterator<UContainer, U>& it2) noexcept;

private:
    const TContainer* _container;
    const T* _node;
};


} // namespace BaseKit

#include "bintree.inl"

#endif // BASEKIT_CONTAINERS_BINTREE_H
