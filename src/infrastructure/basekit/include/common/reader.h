// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_READER_H
#define BASEKIT_READER_H

#include <cstdint>
#include <string>
#include <vector>

namespace BaseKit {

//! Reader interface
/*!
    The Reader interface provides a common interface for reading data from different sources.
    It defines methods for reading bytes, text, and lines of text.
    The implementation of this interface can be used to read data from files, network sockets,
    or any other source that supports reading data in a similar manner.
*/
class Reader
{
public:
    Reader() noexcept = default;
    Reader(const Reader&) noexcept = default;
    Reader(Reader&&) noexcept = default;
    virtual ~Reader() noexcept = default;

    Reader& operator=(const Reader&) noexcept = default;
    Reader& operator=(Reader&&) noexcept = default;

    //! Read a bytes buffer base method
    /*!
        \param buffer - Buffer to read
        \param size - Buffer size
        \return Count of read bytes
    */
    virtual size_t Read(void* buffer, size_t size) = 0;

    //! Read all bytes
    /*!
        \return Bytes buffer
    */
    std::vector<uint8_t> ReadAllBytes();
    //! Read all text
    /*!
        \return Text string
    */
    std::string ReadAllText();
    //! Read all text lines
    /*!
        \return Text lines
    */
    std::vector<std::string> ReadAllLines();
};

} // namespace BaseKit

#endif // BASEKIT_READER_H
