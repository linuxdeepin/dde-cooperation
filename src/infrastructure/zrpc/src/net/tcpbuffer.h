// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_TCPBUFFER_H
#define ZRPC_TCPBUFFER_H

#include <vector>
#include <string>
#include <memory>

namespace zrpc_ns {

class TcpBuffer {
public:
    using ptr = std::shared_ptr<TcpBuffer>;

    TcpBuffer(size_t size = 1024);
    ~TcpBuffer();

    size_t readAvailable() const;
    size_t writeAvailable() const;
    size_t capacity() const;
    size_t getSize() const;

    int readIndex() const;
    int writeIndex() const;

    void ensureWriteSpace(size_t size);
    void write(const void* data, size_t len);
    size_t read(void* data, size_t len);
    void clear();
    void clearBuffer();

    void resizeBuffer(size_t size);
    void writeToBuffer(const char *buf, size_t size);
    void readFromBuffer(std::vector<char> &re, size_t size);
    void adjustBuffer();
    void recycleRead(size_t index);
    void recycleWrite(size_t index);

    std::string getBufferString();
    std::vector<char> getBufferVector();

private:
    std::vector<char> m_buffer;
    size_t m_readIndex;
    size_t m_writeIndex;
};

} // namespace zrpc_ns

#endif // ZRPC_TCPBUFFER_H
