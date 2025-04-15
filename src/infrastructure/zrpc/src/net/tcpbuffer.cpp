// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//#include <unistd.h>
#include <string.h>
#include "tcpbuffer.h"
#include <cstring>
#include "zrpc_log.h"
#include <iostream>

namespace zrpc_ns {

TcpBuffer::TcpBuffer(size_t size) 
    : m_buffer(size)
    , m_readIndex(0)
    , m_writeIndex(0) {
}

TcpBuffer::~TcpBuffer() {
}

size_t TcpBuffer::readAvailable() const {
    return m_writeIndex - m_readIndex;
}

size_t TcpBuffer::writeAvailable() const {
    return m_buffer.size() - m_writeIndex;
}

size_t TcpBuffer::capacity() const {
    return m_buffer.size();
}

int TcpBuffer::readIndex() const {
    return m_readIndex;
}

int TcpBuffer::writeIndex() const {
    return m_writeIndex;
}

void TcpBuffer::ensureWriteSpace(size_t size) {
    if (writeAvailable() < size) {
        size_t newSize = m_buffer.size() * 2;
        while (newSize < m_writeIndex + size) {
            newSize *= 2;
        }
        m_buffer.resize(newSize);
    }
}

void TcpBuffer::write(const void* data, size_t len) {
    ensureWriteSpace(len);
    memcpy(m_buffer.data() + m_writeIndex, data, len);
    m_writeIndex += len;
}

size_t TcpBuffer::read(void* data, size_t len) {
    size_t available = readAvailable();
    if (len > available) {
        len = available;
    }
    if (len > 0) {
        memcpy(data, m_buffer.data() + m_readIndex, len);
        m_readIndex += len;
    }
    return len;
}

void TcpBuffer::clear() {
    m_readIndex = 0;
    m_writeIndex = 0;
}

void TcpBuffer::resizeBuffer(size_t size) {
    std::vector<char> tmp(size);
    size_t c = std::min(size, readAvailable());
    if (m_buffer.size() > m_readIndex) {
        memcpy(tmp.data(), m_buffer.data() + m_readIndex, c);
    }
    m_buffer.swap(tmp);
    m_readIndex = 0;
    m_writeIndex = c;
}

void TcpBuffer::writeToBuffer(const char *buf, size_t size) {
    if (size > writeAvailable()) {
        size_t new_size = static_cast<size_t>(1.5 * (m_writeIndex + size));
        resizeBuffer(new_size);
    }
    memcpy(m_buffer.data() + m_writeIndex, buf, size);
    m_writeIndex += size;
}

void TcpBuffer::readFromBuffer(std::vector<char> &re, size_t size) {
    if (readAvailable() == 0) {
        DLOG << "read buffer empty!";
        return;
    }
    size_t read_size = std::min(readAvailable(), size);
    re.resize(read_size);
    memcpy(re.data(), m_buffer.data() + m_readIndex, read_size);
    m_readIndex += read_size;
    adjustBuffer();
}

void TcpBuffer::adjustBuffer() {
    if (m_readIndex > m_buffer.size() / 3) {
        std::vector<char> new_buffer(m_buffer.size());
        size_t count = readAvailable();
        memcpy(new_buffer.data(), m_buffer.data() + m_readIndex, count);
        m_buffer.swap(new_buffer);
        m_writeIndex = count;
        m_readIndex = 0;
    }
}

size_t TcpBuffer::getSize() const {
    return m_buffer.size();
}

void TcpBuffer::clearBuffer() {
    m_buffer.clear();
    m_readIndex = 0;
    m_writeIndex = 0;
}

void TcpBuffer::recycleRead(size_t index) {
    size_t j = m_readIndex + index;
    if (j > m_buffer.size()) {
        ELOG << "recycleRead error";
        return;
    }
    m_readIndex = j;
    adjustBuffer();
}

void TcpBuffer::recycleWrite(size_t index) {
    size_t j = m_writeIndex + index;
    if (j > m_buffer.size()) {
        ELOG << "recycleWrite error, j=" << j;
        return;
    }
    DLOG << "recycleWrite m_writeIndex=" << m_writeIndex << " j=" << j << " m_buffer.size()=" << m_buffer.size() << std::endl;
    m_writeIndex = j;
    adjustBuffer();
}

std::string TcpBuffer::getBufferString() {
    std::string re(readAvailable(), '0');
    memcpy(&re[0], m_buffer.data() + m_readIndex, readAvailable());
    return re;
}

std::vector<char> TcpBuffer::getBufferVector() {
    return m_buffer;
}

} // namespace zrpc_ns
