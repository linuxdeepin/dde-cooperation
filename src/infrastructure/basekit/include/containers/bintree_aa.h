// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_CONTAINERS_BINTREE_AA_H
#define BASEKIT_CONTAINERS_BINTREE_AA_H

#include "bintree.h"


namespace BaseKit {

template <typename T, typename TCompare = std::less<T>>
class BinTreeAA
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
    typedef BinTreeIterator<BinTreeAA<T, TCompare>, T> iterator;
    typedef BinTreeConstIterator<BinTreeAA<T, TCompare>, T> const_iterator;
    typedef BinTreeReverseIterator<BinTreeAA<T, TCompare>, T> reverse_iterator;
    typedef BinTreeConstReverseIterator<BinTreeAA<T, TCompare>, T> const_reverse_iterator;

    struct Node
    {
        T* parent;
        T* left;
        T* right;
        size_t level;

        Node() : parent(nullptr), left(nullptr), right(nullptr), level(0) {}
    };

    explicit BinTreeAA(const TCompare& compare = TCompare()) noexcept
        : _compare(compare),
          _size(0),
          _root(nullptr)
    {}
    template <class InputIterator>
    BinTreeAA(InputIterator first, InputIterator last, const TCompare& compare = TCompare()) noexcept;
    BinTreeAA(const BinTreeAA&) noexcept = default;
    BinTreeAA(BinTreeAA&&) noexcept = default;
    ~BinTreeAA() noexcept = default;

    BinTreeAA& operator=(const BinTreeAA&) noexcept = default;
    BinTreeAA& operator=(BinTreeAA&&) noexcept = default;

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
    void swap(BinTreeAA& bintree) noexcept;
    template <typename U, typename UCompare>
    friend void swap(BinTreeAA<U, UCompare>& bintree1, BinTreeAA<U, UCompare>& bintree2) noexcept;

private:
    TCompare _compare;  // Binary tree compare
    size_t _size;       // Binary tree size
    T* _root;           // Binary tree root node

    const T* InternalLowest() const noexcept;
    const T* InternalHighest() const noexcept;
    const T* InternalFind(const T& item) const noexcept;
    const T* InternalLowerBound(const T& item) const noexcept;
    const T* InternalUpperBound(const T& item) const noexcept;

    //! Skew the binary tree node
    void Skew(T* node);
    //! Split the binary tree node.
    bool Split(T* node);
};

} // namespace BaseKit

#include "bintree_aa.inl"

#endif // BASEKIT_CONTAINERS_BINTREE_AA_H
