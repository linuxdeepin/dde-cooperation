#include <iostream>
#include "../src/ip/address_v6.h"
#include "../src/ip/network_v6.h"
#include "../src/ip/address.h"

int main() {
    try {
        // 创建 IPv6 地址
        std::cout << "创建 IPv6 地址示例：" << std::endl;
        
        // 创建 IPv6 回环地址
        uasio::ip::address_v6 loopback = uasio::ip::address_v6::loopback();
        std::cout << "IPv6 回环地址: " << loopback.to_string() << std::endl;
        std::cout << "是回环地址: " << (loopback.is_loopback() ? "是" : "否") << std::endl;
        std::cout << "是多播地址: " << (loopback.is_multicast() ? "是" : "否") << std::endl;
        
        // 创建 IPv6 任意地址
        uasio::ip::address_v6 any = uasio::ip::address_v6::any();
        std::cout << "IPv6 任意地址: " << any.to_string() << std::endl;
        
        // 从字符串创建 IPv6 地址
        uasio::ip::address_v6 addr1 = uasio::ip::address_v6::from_string("2001:db8::1");
        std::cout << "地址 2001:db8::1: " << addr1.to_string() << std::endl;
        
        // 创建带范围 ID 的 IPv6 地址
        uasio::ip::address_v6 addr2 = uasio::ip::address_v6::from_string("fe80::1%1");
        std::cout << "链路本地地址 fe80::1%1: " << addr2.to_string() << std::endl;
        std::cout << "范围 ID: " << addr2.scope_id() << std::endl;
        std::cout << "是链路本地地址: " << (addr2.is_link_local() ? "是" : "否") << std::endl;
        
        // 比较 IPv6 地址
        std::cout << "addr1 < addr2: " << (addr1 < addr2 ? "是" : "否") << std::endl;
        std::cout << "addr1 == addr2: " << (addr1 == addr2 ? "是" : "否") << std::endl;
        
        std::cout << "\n创建 IPv6 网络示例：" << std::endl;
        
        // 创建 IPv6 网络
        uasio::ip::network_v6 net1(addr1, 64);
        std::cout << "网络地址: " << net1.address().to_string() << "/" << net1.prefix_length() << std::endl;
        std::cout << "掩码: " << net1.netmask().to_string() << std::endl;
        std::cout << "网络 ID: " << net1.network().to_string() << std::endl;
        
        // 检查地址是否在网络内
        uasio::ip::address_v6 addr3 = uasio::ip::address_v6::from_string("2001:db8::2");
        std::cout << "地址 " << addr3.to_string() << " 在网络 " << net1.network().to_string() 
                  << "/" << net1.prefix_length() << " 内: " 
                  << (net1.contains(addr3) ? "是" : "否") << std::endl;
        
        // 从字符串创建网络
        uasio::ip::network_v6 net2 = uasio::ip::network_v6::from_string("2001:db8:1::/48");
        std::cout << "网络: " << net2.address().to_string() << "/" << net2.prefix_length() << std::endl;
        std::cout << "网络 ID: " << net2.network().to_string() << std::endl;
        
        std::cout << "\n通用 IP 地址示例：" << std::endl;
        
        // 创建通用 IP 地址（IPv6）
        uasio::ip::address ip1(addr1);
        std::cout << "IP 地址: " << ip1.to_string() << std::endl;
        std::cout << "是 IPv6: " << (ip1.is_v6() ? "是" : "否") << std::endl;
        std::cout << "是 IPv4: " << (ip1.is_v4() ? "是" : "否") << std::endl;
        
        // 从字符串创建通用 IP 地址
        uasio::ip::address ip2 = uasio::ip::address::from_string("2001:db8::3");
        std::cout << "从字符串创建 IP 地址: " << ip2.to_string() << std::endl;
        
        // 比较通用 IP 地址
        std::cout << "ip1 < ip2: " << (ip1 < ip2 ? "是" : "否") << std::endl;
        std::cout << "ip1 == ip2: " << (ip1 == ip2 ? "是" : "否") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
    }
    
    return 0;
} 