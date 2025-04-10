#include <iostream>
#include <stdexcept>
#include "../src/multiple_exceptions.h"

// 示例函数，抛出异常
void throw_logic_error() {
    throw std::logic_error("逻辑错误");
}

void throw_runtime_error() {
    throw std::runtime_error("运行时错误");
}

void throw_out_of_range() {
    throw std::out_of_range("索引越界");
}

int main() {
    try {
        // 创建多异常容器
        uasio::multiple_exceptions ex;
        
        std::cout << "捕获多个异常示例：" << std::endl;
        
        // 捕获多个异常
        uasio::capture_exception(throw_logic_error, ex);
        uasio::capture_exception(throw_runtime_error, ex);
        uasio::capture_exception(throw_out_of_range, ex);
        
        // 输出捕获的异常数量
        std::cout << "捕获到 " << ex.size() << " 个异常" << std::endl;
        
        // 尝试获取并处理各个异常
        std::cout << "\n处理每个异常：" << std::endl;
        for (std::size_t i = 0; i < ex.size(); ++i) {
            try {
                std::rethrow_exception(ex.at(i));
            } catch (const std::exception& e) {
                std::cout << "异常 " << (i + 1) << ": " << e.what() << std::endl;
            }
        }
        
        // 使用抛出第一个异常的功能
        std::cout << "\n抛出第一个异常：" << std::endl;
        try {
            ex.throw_first_exception();
        } catch (const std::exception& e) {
            std::cout << "第一个异常: " << e.what() << std::endl;
        }
        
        // 抛出多异常对象本身
        std::cout << "\n抛出多异常对象：" << std::endl;
        uasio::throw_if_exceptions(ex);
        
    } catch (const uasio::multiple_exceptions& ex) {
        std::cout << "捕获到多异常对象: " << ex.what() << std::endl;
        
        // 遍历所有异常
        std::cout << "所有异常：" << std::endl;
        for (const auto& e : ex.exceptions()) {
            try {
                std::rethrow_exception(e);
            } catch (const std::exception& e) {
                std::cout << "- " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "捕获到单一异常: " << e.what() << std::endl;
    }
    
    return 0;
} 