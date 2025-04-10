#ifndef ZRPC_NETADDRESS_H
#define ZRPC_NETADDRESS_H

#include <memory>
#include <string>
#include <asio.hpp>

namespace zrpc_ns {

class NetAddress {
public:
    typedef std::shared_ptr<NetAddress> ptr;

    NetAddress(const char *ip, uint16_t port, char *key, char *crt);
    NetAddress(const char *ip, uint16_t port, bool ssl);

    const char *getIP() const { return m_ip; }
    uint16_t getPort() const { return m_port; }
    bool isSSL() const { return m_ssl; }
    const char *getKey() const { return m_ssl_key; }
    const char *getCrt() const { return m_ssl_crt; }

    std::string toString() const {
        return std::string(m_ip) + ":" + std::to_string(m_port);
    }

private:
    char m_ip[32];
    uint16_t m_port;
    bool m_ssl;
    char m_ssl_key[256];
    char m_ssl_crt[256];
};

} // namespace zrpc_ns

#endif 