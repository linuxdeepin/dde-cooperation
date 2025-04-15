#include <gtest/gtest.h>
#include "memory/memory.h"
#include <vector>
#include <cstring>

using namespace BaseKit;

// 测试内存对齐检查
TEST(MemoryTest, AlignmentValidation) {
    // 测试有效对齐值
    EXPECT_TRUE(Memory::IsValidAlignment(1));
    EXPECT_TRUE(Memory::IsValidAlignment(2));
    EXPECT_TRUE(Memory::IsValidAlignment(4));
    EXPECT_TRUE(Memory::IsValidAlignment(8));
    EXPECT_TRUE(Memory::IsValidAlignment(16));
    EXPECT_TRUE(Memory::IsValidAlignment(32));
    EXPECT_TRUE(Memory::IsValidAlignment(64));
    EXPECT_TRUE(Memory::IsValidAlignment(128));
    EXPECT_TRUE(Memory::IsValidAlignment(256));
    EXPECT_TRUE(Memory::IsValidAlignment(512));
    EXPECT_TRUE(Memory::IsValidAlignment(1024));
    
    // 测试无效对齐值
    EXPECT_FALSE(Memory::IsValidAlignment(0));
    EXPECT_FALSE(Memory::IsValidAlignment(3));
    EXPECT_FALSE(Memory::IsValidAlignment(5));
    EXPECT_FALSE(Memory::IsValidAlignment(6));
    EXPECT_FALSE(Memory::IsValidAlignment(7));
    EXPECT_FALSE(Memory::IsValidAlignment(9));
    EXPECT_FALSE(Memory::IsValidAlignment(15));
}

// 测试指针对齐检查
TEST(MemoryTest, AlignmentCheck) {
    // 创建一个大数组以确保可以获取不同对齐的地址
    alignas(256) char buffer[1024];
    
    // 测试不同对齐值
    for (size_t i = 0; i < 256; ++i) {
        char* ptr = buffer + i;
        
        // 对于每个地址，检查其与各种对齐值的关系
        for (size_t alignment = 1; alignment <= 128; alignment *= 2) {
            bool expected = ((uintptr_t)ptr % alignment) == 0;
            EXPECT_EQ(Memory::IsAligned(ptr, alignment), expected);
        }
    }
}

// 测试指针对齐函数
TEST(MemoryTest, AlignPointer) {
    // 创建一个大数组以确保可以获取不同对齐的地址
    alignas(256) char buffer[1024];
    
    // 测试向上对齐
    for (size_t i = 0; i < 256; ++i) {
        char* ptr = buffer + i;
        
        for (size_t alignment = 1; alignment <= 128; alignment *= 2) {
            char* aligned = Memory::Align(ptr, alignment, true);
            EXPECT_TRUE(Memory::IsAligned(aligned, alignment));
            EXPECT_GE((uintptr_t)aligned, (uintptr_t)ptr); // 向上对齐应该大于等于原地址
        }
    }
    
    // 测试向下对齐
    for (size_t i = 1; i < 256; ++i) {
        char* ptr = buffer + i;
        
        for (size_t alignment = 1; alignment <= 128; alignment *= 2) {
            char* aligned = Memory::Align(ptr, alignment, false);
            EXPECT_TRUE(Memory::IsAligned(aligned, alignment));
            EXPECT_LE((uintptr_t)aligned, (uintptr_t)ptr); // 向下对齐应该小于等于原地址
        }
    }
}

// 测试零填充和零检查
TEST(MemoryTest, ZeroFill) {
    // 准备测试缓冲区
    std::vector<char> buffer(1024);
    
    // 用随机数据填充
    for (size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = static_cast<char>(i & 0xFF);
    }
    
    // 检查是否非零
    EXPECT_FALSE(Memory::IsZero(buffer.data(), buffer.size()));
    
    // 零填充
    Memory::ZeroFill(buffer.data(), buffer.size());
    
    // 检查是否为零
    EXPECT_TRUE(Memory::IsZero(buffer.data(), buffer.size()));
    
    // 手动验证
    for (size_t i = 0; i < buffer.size(); ++i) {
        EXPECT_EQ(buffer[i], 0);
    }
}

// 测试随机填充
TEST(MemoryTest, RandomFill) {
    // 准备测试缓冲区
    std::vector<char> buffer(1024, 0);
    
    // 验证初始状态为零
    EXPECT_TRUE(Memory::IsZero(buffer.data(), buffer.size()));
    
    // 随机填充
    Memory::RandomFill(buffer.data(), buffer.size());
    
    // 检查是否已被随机填充（统计学检查）
    // 注意：理论上有可能随机生成全0，但概率极低
    EXPECT_FALSE(Memory::IsZero(buffer.data(), buffer.size()));
    
    // 检查是否有足够的不同值以确认随机性
    std::vector<bool> valuesSeen(256, false);
    int uniqueValues = 0;
    
    for (size_t i = 0; i < buffer.size(); ++i) {
        unsigned char val = static_cast<unsigned char>(buffer[i]);
        if (!valuesSeen[val]) {
            valuesSeen[val] = true;
            uniqueValues++;
        }
    }
    
    // 在1024个字节中，应该至少有几十个不同的值
    EXPECT_GT(uniqueValues, 30);
}

// 测试加密随机填充
TEST(MemoryTest, CryptoFill) {
    // 准备测试缓冲区
    std::vector<char> buffer(1024, 0);
    
    // 验证初始状态为零
    EXPECT_TRUE(Memory::IsZero(buffer.data(), buffer.size()));
    
    // 加密随机填充
    Memory::CryptoFill(buffer.data(), buffer.size());
    
    // 检查是否已被随机填充（统计学检查）
    EXPECT_FALSE(Memory::IsZero(buffer.data(), buffer.size()));
    
    // 检查是否有足够的不同值以确认随机性（与普通随机相同）
    std::vector<bool> valuesSeen(256, false);
    int uniqueValues = 0;
    
    for (size_t i = 0; i < buffer.size(); ++i) {
        unsigned char val = static_cast<unsigned char>(buffer[i]);
        if (!valuesSeen[val]) {
            valuesSeen[val] = true;
            uniqueValues++;
        }
    }
    
    // 在1024个字节中，应该至少有几十个不同的值
    EXPECT_GT(uniqueValues, 30);
}

// 测试系统内存信息
TEST(MemoryTest, SystemMemory) {
    // 测试总内存
    int64_t total = Memory::RamTotal();
    EXPECT_GT(total, 0);
    
    // 测试可用内存
    int64_t free = Memory::RamFree();
    EXPECT_GT(free, 0);
    
    // 可用内存应该小于等于总内存
    EXPECT_LE(free, total);
    
    // 输出内存信息
    std::cout << "Total RAM: " << total << " bytes (" << (total / (1024 * 1024)) << " MB)" << std::endl;
    std::cout << "Free RAM: " << free << " bytes (" << (free / (1024 * 1024)) << " MB)" << std::endl;
} 