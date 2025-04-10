#include <iostream>
#include "../src/ip/address_v4.h"
#include "../src/ip/network_v4.h"
#include "../src/ip/address.h"

int main() {
    try {
        // 创建 IPv4 地址
        std::cout << "创建 IPv4 地址示例：" << std::endl;
        
        // 创建 IPv4 回环地址
        uasio::ip::address_v4 loopback = uasio::ip::address_v4::loopback();
        std::cout << "IPv4 回环地址: " << loopback.to_string() << std::endl;
        std::cout << "是回环地址: " << (loopback.is_loopback() ? "是" : "否") << std::endl;
        std::cout << "是多播地址: " << (loopback.is_multicast() ? "是" : "否") << std::endl;
        
        // 创建 IPv4 任意地址
        uasio::ip::address_v4 any = uasio::ip::address_v4::any();
        std::cout << "IPv4 任意地址: " << any.to_string() << std::endl;
        
        // 创建 IPv4 广播地址
        uasio::ip::address_v4 broadcast = uasio::ip::address_v4::broadcast();
        std::cout << "IPv4 广播地址: " << broadcast.to_string() << std::endl;
        
        // 从字符串创建 IPv4 地址
        uasio::ip::address_v4 addr1 = uasio::ip::address_v4::from_string("192.168.1.1");
        std::cout << "地址 192.168.1.1: " << addr1.to_string() << std::endl;
        
        // 从字节数组创建 IPv4 地址
        uasio::ip::address_v4::bytes_type bytes = {{10, 0, 0, 1}};
        uasio::ip::address_v4 addr2(bytes);
        std::cout << "地址 10.0.0.1: " << addr2.to_string() << std::endl;
        
        // 从整数创建 IPv4 地址
        uasio::ip::address_v4 addr3(0xC0A80101); // 192.168.1.1
        std::cout << "从整数创建的地址: " << addr3.to_string() << std::endl;
        
        // 比较 IPv4 地址
        std::cout << "addr1 == addr3: " << (addr1 == addr3 ? "是" : "否") << std::endl;
        std::cout << "addr1 < addr2: " << (addr1 < addr2 ? "是" : "否") << std::endl;
        
        std::cout << "\n创建 IPv4 网络示例：" << std::endl;
        
        // 创建 IPv4 网络
        uasio::ip::network_v4 net1(addr1, 24);
        std::cout << "网络地址: " << net1.address().to_string() << "/" << net1.prefix_length() << std::endl;
        std::cout << "掩码: " << net1.netmask().to_string() << std::endl;
        std::cout << "网络 ID: " << net1.network().to_string() << std::endl;
        std::cout << "广播地址: " << net1.broadcast().to_string() << std::endl;
        
        // 检查地址是否在网络内
        uasio::ip::address_v4 addr4 = uasio::ip::address_v4::from_string("192.168.1.100");
        std::cout << "地址 " << addr4.to_string() << " 在网络 " << net1.network().to_string() 
                  << "/" << net1.prefix_length() << " 内: " 
                  << (net1.contains(addr4) ? "是" : "否") << std::endl;
        
        // 从字符串创建网络
        uasio::ip::network_v4 net2 = uasio::ip::network_v4::from_string("10.0.0.0/8");
        std::cout << "网络: " << net2.address().to_string() << "/" << net2.prefix_length() << std::endl;
        std::cout << "网络 ID: " << net2.network().to_string() << std::endl;
        std::cout << "广播地址: " << net2.broadcast().to_string() << std::endl;
        
        std::cout << "\n通用 IP 地址示例：" << std::endl;
        
        // 创建通用 IP 地址（IPv4）
        uasio::ip::address ip1(addr1);
        std::cout << "IP 地址: " << ip1.to_string() << std::endl;
        std::cout << "是 IPv4: " << (ip1.is_v4() ? "是" : "否") << std::endl;
        std::cout << "是 IPv6: " << (ip1.is_v6() ? "是" : "否") << std::endl;
        
        // 从字符串创建通用 IP 地址
        uasio::ip::address ip2 = uasio::ip::address::from_string("127.0.0.1");
        std::cout << "从字符串创建 IP 地址: " << ip2.to_string() << std::endl;
        std::cout << "是回环地址: " << (ip2.is_loopback() ? "是" : "否") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
    }
    
    return 0;
} 