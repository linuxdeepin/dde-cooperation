#include "src/buffer.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <array>
#include <string>

// 十六进制打印缓冲区内容
void print_hex(const void* data, std::size_t size) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    
    for (std::size_t i = 0; i < size; ++i) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(p[i]) << " ";
        
        if ((i + 1) % 16 == 0 || i == size - 1) {
            std::cout << "  ";
            
            // 打印ASCII
            for (std::size_t j = (i / 16) * 16; j <= i; ++j) {
                unsigned char c = p[j];
                if (c >= 32 && c <= 126) {
                    std::cout << c;
                } else {
                    std::cout << ".";
                }
            }
            
            std::cout << std::endl;
        }
    }
    
    std::cout << std::dec; // 恢复十进制输出
}

int main() {
    try {
        std::cout << "缓冲区 (buffer) 测试程序" << std::endl;
        std::cout << "====================" << std::endl;
        
        // 测试从原始指针创建缓冲区
        {
            std::cout << "1. 从原始指针创建缓冲区" << std::endl;
            
            const char* data = "Hello, 这是测试数据";
            std::size_t size = std::strlen(data);
            
            asio::const_buffer buf = asio::buffer(data, size);
            
            std::cout << "  缓冲区大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  缓冲区内容: ";
            print_hex(buf.data(), buf.size());
            std::cout << std::endl;
        }
        
        // 测试从std::string创建缓冲区
        {
            std::cout << "2. 从std::string创建缓冲区" << std::endl;
            
            std::string str = "从字符串创建的缓冲区";
            
            asio::const_buffer buf = asio::buffer(str);
            
            std::cout << "  字符串大小: " << str.size() << " 字节" << std::endl;
            std::cout << "  缓冲区大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  缓冲区内容: ";
            print_hex(buf.data(), buf.size());
            std::cout << std::endl;
        }
        
        // 测试从std::vector创建缓冲区
        {
            std::cout << "3. 从std::vector创建缓冲区" << std::endl;
            
            std::vector<char> vec = {'V', 'e', 'c', 't', 'o', 'r', ' ', 'T', 'e', 's', 't'};
            
            asio::mutable_buffer buf = asio::buffer(vec);
            
            std::cout << "  向量大小: " << vec.size() << " 元素" << std::endl;
            std::cout << "  缓冲区大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  缓冲区内容: ";
            print_hex(buf.data(), buf.size());
            
            // 修改缓冲区内容
            char* data = static_cast<char*>(buf.data());
            data[0] = 'v';  // 小写v
            
            std::cout << "  修改后内容: ";
            print_hex(buf.data(), buf.size());
            std::cout << std::endl;
        }
        
        // 测试从std::array创建缓冲区
        {
            std::cout << "4. 从std::array创建缓冲区" << std::endl;
            
            std::array<int, 5> arr = {10, 20, 30, 40, 50};
            
            asio::const_buffer buf = asio::buffer(arr);
            
            std::cout << "  数组大小: " << arr.size() << " 元素" << std::endl;
            std::cout << "  缓冲区大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  缓冲区内容: ";
            print_hex(buf.data(), buf.size());
            std::cout << std::endl;
        }
        
        // 测试缓冲区偏移
        {
            std::cout << "5. 测试缓冲区偏移" << std::endl;
            
            const char* data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            std::size_t size = std::strlen(data);
            
            asio::const_buffer buf = asio::buffer(data, size);
            
            std::cout << "  原始缓冲区: ";
            print_hex(buf.data(), buf.size());
            
            // 偏移5个字节
            asio::const_buffer offset_buf = buf + 5;
            
            std::cout << "  偏移5字节后: ";
            print_hex(offset_buf.data(), offset_buf.size());
            
            // 偏移超过大小
            asio::const_buffer invalid_buf = buf + 100;
            
            std::cout << "  偏移100字节后 (无效): ";
            std::cout << "大小 = " << invalid_buf.size() << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "缓冲区测试程序完成!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 