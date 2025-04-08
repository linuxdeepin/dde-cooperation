// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETUTIL_ASIO_H
#define NETUTIL_ASIO_H

#include <iostream>

#if defined(__clang__)
#pragma clang system_header
#elif defined(__GNUC__)
#pragma GCC system_header
#elif defined(_MSC_VER)
#pragma system_header
#endif

#if defined(__CYGWIN__)

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#else

#define ASIO_STANDALONE
#define ASIO_SEPARATE_COMPILATION
#define ASIO_NO_WIN32_LEAN_AND_MEAN

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4459) // C4459: declaration of 'identifier' hides global declaration
#endif

#include <asio.hpp>
#include <asio/ssl.hpp>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_WIN32) || defined(_WIN64)
#undef DELETE
#undef ERROR
#undef Yield
#undef min
#undef max
#undef uuid_t
#endif

#endif

namespace NetUtil {

/*!
    \namespace NetUtil::Asio
    \brief Asio definitions
*/
namespace Asio {

//! Internet protocol
enum class InternetProtocol
{
    IPv4,               //!< Internet Protocol version 4
    IPv6                //!< Internet Protocol version 6
};

//! Stream output: Internet protocol
/*!
    \param stream - Output stream
    \param protocol - Internet protocol
    \return Output stream
*/
template <class TOutputStream>
TOutputStream& operator<<(TOutputStream& stream, InternetProtocol protocol);

} // namespace Asio
} // namespace NetUtil

#include "asio.inl"

#endif // NETUTIL_ASIO_H
