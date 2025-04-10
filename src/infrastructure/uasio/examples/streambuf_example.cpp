// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../src/asio.h"
#include <iostream>
#include <string>
#include <iomanip>

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
        std::cout << "流缓冲区 (streambuf) 示例程序" << std::endl;
        std::cout << "==========================" << std::endl;
        
        // 创建缓冲区
        asio::streambuf buf;
        
        std::cout << "初始状态:" << std::endl;
        std::cout << "  大小: " << buf.size() << " 字节" << std::endl;
        std::cout << "  容量: " << buf.capacity() << " 字节" << std::endl;
        std::cout << "  是否为空: " << (buf.empty() ? "是" : "否") << std::endl;
        std::cout << std::endl;
        
        // 写入一些数据
        {
            std::cout << "准备写入数据..." << std::endl;
            std::string data = "Hello, 这是一个测试消息!";
            
            // 获取写入缓冲区
            asio::mutable_buffer write_buf = buf.prepare(data.size());
            
            // 复制数据
            std::memcpy(write_buf.data(), data.data(), data.size());
            
            // 确认写入
            buf.commit(data.size());
            
            std::cout << "  已写入: " << data.size() << " 字节" << std::endl;
            std::cout << "  当前大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  当前容量: " << buf.capacity() << " 字节" << std::endl;
            std::cout << "  内容: " << buf.str() << std::endl;
            std::cout << std::endl;
        }
        
        // 以十六进制打印缓冲区内容
        {
            std::cout << "缓冲区内容 (十六进制):" << std::endl;
            asio::const_buffer data = buf.data();
            print_hex(data.data(), data.size());
            std::cout << std::endl;
        }
        
        // 读取一部分数据
        {
            std::cout << "读取一部分数据..." << std::endl;
            asio::const_buffer data = buf.data();
            std::size_t to_read = 10; // 读取前10个字节
            
            std::cout << "  读取: " << to_read << " 字节" << std::endl;
            
            // 复制数据
            std::vector<char> read_data(to_read);
            std::memcpy(read_data.data(), data.data(), to_read);
            
            // 消费数据
            buf.consume(to_read);
            
            std::cout << "  读取内容: ";
            for (char c : read_data) {
                std::cout << c;
            }
            std::cout << std::endl;
            
            std::cout << "  剩余大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  剩余内容: " << buf.str() << std::endl;
            std::cout << std::endl;
        }
        
        // 追加更多数据
        {
            std::cout << "追加更多数据..." << std::endl;
            std::string more_data = " [这是追加的更多数据]";
            
            // 获取写入缓冲区
            asio::mutable_buffer write_buf = buf.prepare(more_data.size());
            
            // 复制数据
            std::memcpy(write_buf.data(), more_data.data(), more_data.size());
            
            // 确认写入
            buf.commit(more_data.size());
            
            std::cout << "  已追加: " << more_data.size() << " 字节" << std::endl;
            std::cout << "  当前大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  内容: " << buf.str() << std::endl;
            std::cout << std::endl;
        }
        
        // 使用 buffer() 函数
        {
            std::cout << "使用 buffer() 函数..." << std::endl;
            
            // 从字符串创建缓冲区
            std::string str = "通过字符串创建缓冲区";
            asio::const_buffer str_buf = asio::buffer(str);
            
            std::cout << "  字符串缓冲区大小: " << str_buf.size() << " 字节" << std::endl;
            
            // 从数组创建缓冲区
            std::array<char, 10> arr = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
            asio::mutable_buffer arr_buf = asio::buffer(arr);
            
            std::cout << "  数组缓冲区大小: " << arr_buf.size() << " 字节" << std::endl;
            
            // 从向量创建缓冲区
            std::vector<int> vec = {10, 20, 30, 40, 50};
            asio::const_buffer vec_buf = asio::buffer(vec);
            
            std::cout << "  向量缓冲区大小: " << vec_buf.size() << " 字节" << std::endl;
            std::cout << std::endl;
        }
        
        // 测试缓冲区大小调整
        {
            std::cout << "测试缓冲区大小调整..." << std::endl;
            
            // 创建小缓冲区
            asio::streambuf small_buf(100); // 最大100字节
            
            std::cout << "  小缓冲区最大大小: " << small_buf.max_size() << " 字节" << std::endl;
            
            // 尝试写入大量数据
            try {
                std::string large_data(200, 'X'); // 200字节数据
                small_buf.prepare(large_data.size());
                std::cout << "  错误: 应该抛出异常!" << std::endl;
            } catch (const std::length_error& e) {
                std::cout << "  捕获到异常: " << e.what() << std::endl;
            }
            
            // 写入适当大小的数据
            std::string ok_data(50, 'Y'); // 50字节数据
            asio::mutable_buffer write_buf = small_buf.prepare(ok_data.size());
            std::memcpy(write_buf.data(), ok_data.data(), ok_data.size());
            small_buf.commit(ok_data.size());
            
            std::cout << "  成功写入: " << small_buf.size() << " 字节" << std::endl;
            std::cout << std::endl;
        }
        
        // 清空缓冲区
        {
            std::cout << "清空缓冲区..." << std::endl;
            
            std::cout << "  清空前大小: " << buf.size() << " 字节" << std::endl;
            buf.clear();
            std::cout << "  清空后大小: " << buf.size() << " 字节" << std::endl;
            std::cout << "  是否为空: " << (buf.empty() ? "是" : "否") << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "流缓冲区示例程序完成!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 