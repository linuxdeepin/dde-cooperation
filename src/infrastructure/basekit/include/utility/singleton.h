// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_UTILITY_SINGLETON_H
#define BASEKIT_UTILITY_SINGLETON_H

namespace BaseKit {

//! Singleton template base class
/*!
    Singleton base class is used to protect child class from being created
    multiple times and provide a static GetInstance() method to access the
    single instance.

    Thread-safe.

    Example:
    \code{.cpp}
    class MySingleton : public BaseKit::Singleton<MySingleton>
    {
       friend BaseKit::Singleton<MySingleton>;

    public:
        void Test() { ... }

    private:
        MySingleton() = default;
        ~MySingleton() = default;
    };

    int main(int argc, char** argv)
    {
        MySingleton::GetInstance().Test();
        return 0;
    }
    \endcode

    https://en.wikipedia.org/wiki/Singleton_pattern
*/
template <typename T>
class Singleton
{
    friend T;

public:
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;

    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton &&) = delete;

    //! Get singleton instance
    /*!
        \return Singleton instance
    */
    static T& GetInstance()
    {
        static T instance;
        return instance;
    }

private:
    Singleton() noexcept = default;
    ~Singleton() noexcept = default;
};


} // namespace BaseKit

#endif // BASEKIT_UTILITY_SINGLETON_H
