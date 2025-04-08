// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace BaseKit {

template<typename T>
inline WaitQueue<T>::WaitQueue(size_t capacity) : _closed(false),  _capacity(capacity)
{
}

template<typename T>
inline WaitQueue<T>::~WaitQueue()
{
    Close();
}

template<typename T>
inline bool WaitQueue<T>::closed() const
{
    Locker<CriticalSection> locker(_cs);
    return _closed;
}

template<typename T>
inline size_t WaitQueue<T>::capacity() const
{
    if (_capacity > 0)
        return _capacity;

    Locker<CriticalSection> locker(_cs);
    return _queue.size();
}

template<typename T>
inline size_t WaitQueue<T>::size() const
{
    Locker<CriticalSection> locker(_cs);
    return _queue.size();
}

template<typename T>
inline bool WaitQueue<T>::Enqueue(const T& item)
{
    Locker<CriticalSection> locker(_cs);

    if (_closed)
        return false;

    do
    {
        if ((_capacity == 0) || (_queue.size() < _capacity))
        {
            _queue.push(item);
            _cv1.NotifyOne();
            return true;
        }

        _cv2.Wait(_cs, [this]() { return (_closed || (_capacity == 0) || (_queue.size() < _capacity)); });

    } while (!_closed);

    return false;
}

template<typename T>
inline bool WaitQueue<T>::Enqueue(T&& item)
{
    Locker<CriticalSection> locker(_cs);

    if (_closed)
        return false;

    do
    {
        if ((_capacity == 0) || (_queue.size() < _capacity))
        {
            _queue.emplace(item);
            _cv1.NotifyOne();
            return true;
        }

        _cv2.Wait(_cs, [this]() { return (_closed || (_capacity == 0) || (_queue.size() < _capacity)); });

    } while (!_closed);

    return false;
}

template<typename T>
inline bool WaitQueue<T>::Dequeue(T& item)
{
    Locker<CriticalSection> locker(_cs);

    if (_closed && _queue.empty())
        return false;

    do
    {
        if (!_queue.empty())
        {
            item = _queue.front();
            _queue.pop();
            _cv2.NotifyOne();
            return true;
        }

        _cv1.Wait(_cs, [this]() { return (_closed || !_queue.empty()); });

    } while (!_closed || !_queue.empty());

    return false;
}

template<typename T>
inline void WaitQueue<T>::Close()
{
    Locker<CriticalSection> locker(_cs);
    _closed = true;
    _cv1.NotifyAll();
    _cv2.NotifyAll();
}

} // namespace BaseKit
