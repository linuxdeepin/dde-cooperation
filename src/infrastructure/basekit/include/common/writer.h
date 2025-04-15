// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BASEKIT_WRITER_H
#define BASEKIT_WRITER_H
#include <string>
#include <vector>

namespace BaseKit {

//! Writer interface
/*!
    The Writer interface provides a common interface for writing data to different destinations.
    It defines methods for writing bytes, text, and lines of text.
    The implementation of this interface can be used to write data to files, network sockets,
    or any other destination that supports writing data in a similar manner.
*/
class Writer
{
public:
    Writer() noexcept = default;
    Writer(const Writer&) noexcept = default;
    Writer(Writer&&) noexcept = default;
    virtual ~Writer() noexcept = default;

    Writer& operator=(const Writer&) noexcept = default;
    Writer& operator=(Writer&&) noexcept = default;

    //! Write a byte buffer
    /*!
        \param buffer - Pointer to the buffer to write
        \param size - Size of the buffer in bytes
        \return The number of bytes written
    */
    virtual size_t Write(const void* buffer, size_t size) = 0;

    //! Write a text string
    /*!
        \param text - The text string to write
        \return The number of characters written
    */
    size_t Write(const std::string& text);
    //! Write text lines
    /*!
        \param lines - Text lines
        \return Count of written lines
    */
    size_t Write(const std::vector<std::string>& lines);

    //! Flush the writer
    virtual void Flush() {}
};

} // namespace BaseKit

#endif // BASEKIT_WRITER_H
