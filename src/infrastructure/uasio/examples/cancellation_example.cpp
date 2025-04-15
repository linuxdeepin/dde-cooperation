#include <iostream>
#include <thread>
#include <chrono>
#include "../src/cancellation_signal.h"

void longRunningTask(uasio::cancellation_signal& signal) {
    // 创建取消处理槽
    auto slot = signal.slot();
    
    // 为槽分配处理函数
    slot.assign([](uasio::cancellation_type type) {
        if (uasio::contains_type(type, uasio::cancellation_type::terminal)) {
            std::cout << "任务收到终止信号！" << std::endl;
        } else if (uasio::contains_type(type, uasio::cancellation_type::partial)) {
            std::cout << "任务收到部分取消信号！" << std::endl;
        }
    });
    
    // 模拟长时间运行的任务
    std::cout << "开始执行长时间任务..." << std::endl;
    
    for (int i = 0; i < 10; ++i) {
        std::cout << "任务执行中: " << (i + 1) << "/10" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 检查是否有取消信号（这里只是示例，实际中应该更复杂）
        if (slot.connected()) {
            std::cout << "检查取消状态：槽仍然连接" << std::endl;
        } else {
            std::cout << "检查取消状态：槽已断开" << std::endl;
            break;
        }
    }
    
    std::cout << "任务完成！" << std::endl;
}

int main() {
    // 创建取消信号
    uasio::cancellation_signal signal;
    
    // 在单独的线程中执行长时间任务
    std::thread taskThread(longRunningTask, std::ref(signal));
    
    // 等待3秒后发送部分取消信号
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "发送部分取消信号..." << std::endl;
    signal.emit(uasio::cancellation_type::partial);
    
    // 再等待3秒后发送终止信号
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "发送终止信号..." << std::endl;
    signal.emit(uasio::cancellation_type::terminal);
    
    // 等待任务线程完成
    if (taskThread.joinable()) {
        taskThread.join();
    }
    
    std::cout << "主程序退出" << std::endl;
    return 0;
} 