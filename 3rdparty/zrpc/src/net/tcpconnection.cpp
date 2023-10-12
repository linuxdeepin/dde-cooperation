// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//#include <unistd.h>
#include <string.h>
//#include <sys/socket.h>
#include "tcpconnection.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include "specodec.h"
#include "specdata.h"
#include "co/co.h"

namespace zrpc_ns {

TcpConnection::TcpConnection(TcpServer *tcp_svr,
                             tcp::Connection *serv_conn,
                             int buff_size,
                             NetAddress::ptr peer_addr)
    : m_state(Connected), m_connection_type(ServerConnection), m_peer_addr(peer_addr)
{

    m_tcp_svr = tcp_svr;
    m_serv_conn = serv_conn;

    m_codec = m_tcp_svr->getCodec();
    initBuffer(buff_size);
    m_state = Connected;
    DLOG << "succ create tcp connection[" << m_state << "]";
}

TcpConnection::TcpConnection(TcpClient *tcp_cli,
                             tcp::Client *cli_conn,
                             int buff_size,
                             NetAddress::ptr peer_addr)
    : m_state(NotConnected), m_connection_type(ClientConnection), m_peer_addr(peer_addr)
{

    m_tcp_cli = tcp_cli;
    m_cli_conn = cli_conn;

    m_codec = m_tcp_cli->getCodeC();

    initBuffer(buff_size);

    DLOG << "succ create tcp connection[NotConnected]";
}

void TcpConnection::initServer()
{
    this->MainServerLoopCorFunc();
}

void TcpConnection::setUpClient()
{
    setState(Connected);
}

TcpConnection::~TcpConnection()
{
    DLOG << "~TcpConnection";
}

void TcpConnection::initBuffer(int size)
{

    // 初始化缓冲区大小
    m_write_buffer = std::make_shared<TcpBuffer>(size);
    m_read_buffer = std::make_shared<TcpBuffer>(size);
}

int64 TcpConnection::read_hook(char *buf, size_t len)
{
    int64 r = 0;

    if (m_connection_type == ServerConnection) {
        if (m_serv_conn) {
            r = m_serv_conn->recv(buf, len);
        } else {
            ELOG << "TcpConnection::read_hook NULL m_serv_conn";
        }
    } else if (m_connection_type == ClientConnection) {
        if (m_cli_conn) {
            r = m_cli_conn->recv(buf, len);
        } else {
            ELOG << "TcpConnection::read_hook NULL m_cli_conn";
        }
    }

    return r;
}

int64 TcpConnection::write_hook(const void *buf, size_t count)
{
    int64 r = 0;
    if (m_connection_type == ServerConnection) {
        if (m_serv_conn) {
            r = m_serv_conn->send(buf, count, m_trans_timeout);
        } else {
            ELOG << "TcpConnection::write_hook NULL m_serv_conn";
        }
    } else if (m_connection_type == ClientConnection) {
        if (m_cli_conn) {
            r = m_cli_conn->send(buf, count, m_trans_timeout);
        } else {
            ELOG << "TcpConnection::write_hook NULL m_cli_conn";
        }
    }

    return r;
}

void TcpConnection::MainServerLoopCorFunc()
{

    while (!m_stop) {
        input();

        execute();

        output();
    }
    LOG << "this connection has already end loop";
}

void TcpConnection::input()
{
    TcpConnectionState state = getState();
    if (state == Closed || state == NotConnected) {
        return;
    }

    bool read_all = false;
    bool close_flag = false;
    int count = 0;
    while (!read_all) {

        if (m_read_buffer->writeAble() == 0) {
            m_read_buffer->resizeBuffer(2 * m_read_buffer->getSize());
        }

        int read_count = m_read_buffer->writeAble();
        int write_index = m_read_buffer->writeIndex();

        // DLOG << "m_read_buffer size=" << m_read_buffer->getBufferVector().size()
        //      << " rd=" << m_read_buffer->readIndex() << " wd=" << m_read_buffer->writeIndex();
        int rt = read_hook(&(m_read_buffer->m_buffer[write_index]), read_count);
        if (rt > 0) {
            m_read_buffer->recycleWrite(rt);
        }
        // DLOG << "m_read_buffer size=" << m_read_buffer->getBufferVector().size()
        //      << " rd=" << m_read_buffer->readIndex() << " wd=" << m_read_buffer->writeIndex();

        count += rt;
        if (rt <= 0) {
            DLOG << "rt <= 0 >>> " << rt;
//             ELOG << "read empty while occur read event, because of peer close, sys error="
//                  << strerror(errno) << ", now to clear tcp connection";
//            // this cor can destroy
            close_flag = true;
            break;
        } else {
            if (rt == read_count) {
                // is is possible read more data, should continue read
                continue;
            } else if (rt < read_count) {
                // read all data in socket buffer, skip out loop
                read_all = true;
                break;
            }
        }
    }
    if (close_flag) {
        clearClient();
        DLOG << "peer closed";
        return;
    }

    if (!read_all) {
        ELOG << "not read all data in socket buffer, but end! recv [" << count << "]";
    }
}

void TcpConnection::execute()
{
    // it only server do this
    while (m_read_buffer->readAble() > 0) {
        std::shared_ptr<AbstractData> data;
        data = std::make_shared<SpecDataStruct>();

        m_codec->decode(m_read_buffer.get(), data.get());
        // DLOG << "parse service_name=" << pb_struct.service_full_name;
        if (!data->decode_succ) {
            ELOG << "it parse request error";
            break;
        }

        if (m_connection_type == ServerConnection) {
            m_tcp_svr->getDispatcher()->dispatch(data.get(), this);
        } else if (m_connection_type == ClientConnection) {
            std::shared_ptr<SpecDataStruct> tmp = std::dynamic_pointer_cast<SpecDataStruct>(data);
            if (tmp) {
                m_reply_datas.insert(std::make_pair(tmp->msg_req, tmp));
            }
        }
    }
}

void TcpConnection::output()
{
    while (true) {
        TcpConnectionState state = getState();
        if (state != Connected) {
            break;
        }

        if (m_write_buffer->readAble() == 0) {
            DLOG << "app buffer no data to write, to yiled this coroutine";
            break;
        }

        int total_size = m_write_buffer->readAble();
        int read_index = m_write_buffer->readIndex();
        int rt = write_hook(&(m_write_buffer->m_buffer[read_index]), total_size);
        if (rt <= 0) {
            ELOG << "write empty, error=" << strerror(errno);
            break;
        }

        // DLOG << "succ write " << rt << " bytes";
        m_write_buffer->recycleRead(rt);
        // DLOG << "recycle write index =" << m_write_buffer->writeIndex()
        //      << ", read_index =" << m_write_buffer->readIndex()
        //      << "readable = " << m_write_buffer->readAble();
        // LOG << "send[" << rt << "] bytes data to [" << m_peer_addr->toString() << "]";
        if (m_write_buffer->readAble() <= 0) {
            // LOG << "send all data, now unregister write event and break";
            break;
        }
    }
}

void TcpConnection::clearClient()
{
    if (getState() == Closed) {
        DLOG << "this client has closed";
        return;
    }

    // stop read and write cor
    m_stop = true;

    if (m_serv_conn) {
        m_serv_conn->reset(3000);
    }

    if (m_cli_conn) {
        m_cli_conn->close();
    }

    setState(Closed);
}

void TcpConnection::shutdownConnection()
{
    TcpConnectionState state = getState();
    if (state == Closed || state == NotConnected) {
        DLOG << "this client has closed";
        return;
    }
    setState(HalfClosing);
    LOG << "shutdown conn[" << m_peer_addr->toString() << "]";
}

TcpBuffer *TcpConnection::getInBuffer()
{
    return m_read_buffer.get();
}

TcpBuffer *TcpConnection::getOutBuffer()
{
    return m_write_buffer.get();
}

bool TcpConnection::getResPackageData(const std::string &msg_req,
                                      SpecDataStruct::pb_ptr &pb_struct)
{
    auto it = m_reply_datas.find(msg_req);
    if (it != m_reply_datas.end()) {
        // DLOG << "return a res data";
        pb_struct = it->second;
        m_reply_datas.erase(it);
        return true;
    }
    // DLOG << msg_req << "|reply data not exist";
    return false;
}

AbstractCodeC::ptr TcpConnection::getCodec() const
{
    return m_codec;
}

TcpConnectionState TcpConnection::getState()
{
    return m_state;
}

void TcpConnection::setState(const TcpConnectionState &state)
{
    m_state = state;
}

}   // namespace zrpc_ns
