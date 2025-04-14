#include <gtest/gtest.h>
#include "cache/memcache.h"
#include "time/timespan.h"
#include <string>
#include <thread>

using namespace BaseKit;

// 测试基本内存缓存操作
TEST(MemCacheTest, BasicOperations) {
    // 创建一个内存缓存实例
    MemCache<std::string, int> cache;
    
    // 验证新创建的缓存为空
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0);
    EXPECT_FALSE(bool(cache));
    
    // 插入值到缓存
    EXPECT_TRUE(cache.insert("key1", 100));
    EXPECT_TRUE(cache.insert("key2", 200));
    
    // 验证缓存不再为空
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(cache.size(), 2);
    EXPECT_TRUE(bool(cache));
    
    // 查找缓存中的值
    EXPECT_TRUE(cache.find("key1"));
    EXPECT_TRUE(cache.find("key2"));
    EXPECT_FALSE(cache.find("key3"));
    
    // 获取缓存中的值
    int value;
    EXPECT_TRUE(cache.find("key1", value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(cache.find("key2", value));
    EXPECT_EQ(value, 200);
    EXPECT_FALSE(cache.find("key3", value));
    
    // 移除缓存中的值
    EXPECT_TRUE(cache.remove("key1"));
    EXPECT_FALSE(cache.find("key1"));
    EXPECT_TRUE(cache.find("key2"));
    EXPECT_EQ(cache.size(), 1);
    
    // 清空缓存
    cache.clear();
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0);
}

// 测试emplace操作
TEST(MemCacheTest, EmplaceOperation) {
    MemCache<int, std::string> cache;
    
    // 使用emplace添加值
    EXPECT_TRUE(cache.emplace(100, "value1"));
    EXPECT_TRUE(cache.emplace(200, "value2"));
    
    // 验证值正确存储
    std::string value;
    EXPECT_TRUE(cache.find(100, value));
    EXPECT_EQ(value, "value1");
    EXPECT_TRUE(cache.find(200, value));
    EXPECT_EQ(value, "value2");
}

// 测试超时功能
TEST(MemCacheTest, TimeoutFeature) {
    MemCache<std::string, int> cache;
    
    // 添加有超时的值
    EXPECT_TRUE(cache.insert("key1", 100, Timespan::milliseconds(100))); // 100毫秒后过期
    EXPECT_TRUE(cache.insert("key2", 200, Timespan::seconds(10))); // 10秒后过期
    
    // 验证值存在
    EXPECT_TRUE(cache.find("key1"));
    EXPECT_TRUE(cache.find("key2"));
    
    // 等待第一个值过期
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // 触发watchdog机制
    cache.watchdog();
    
    // 验证第一个值已过期，第二个值仍然存在
    EXPECT_FALSE(cache.find("key1"));
    EXPECT_TRUE(cache.find("key2"));
    
    // 验证缓存大小
    EXPECT_EQ(cache.size(), 1);
}

// 测试超时值查询
TEST(MemCacheTest, TimeoutQuery) {
    MemCache<std::string, int> cache;
    
    // 添加具有不同超时的值
    EXPECT_TRUE(cache.insert("key1", 100, Timespan::seconds(5)));
    
    // 查询值和其超时时间
    int value;
    Timestamp timeout;
    EXPECT_TRUE(cache.find("key1", value, timeout));
    EXPECT_EQ(value, 100);
    
    // 验证超时时间在将来
    EXPECT_GT(timeout, UtcTimestamp());
}

// 测试缓存交换
TEST(MemCacheTest, SwapOperation) {
    MemCache<std::string, int> cache1;
    MemCache<std::string, int> cache2;
    
    // 填充第一个缓存
    cache1.insert("key1", 100);
    cache1.insert("key2", 200);
    
    // 填充第二个缓存
    cache2.insert("key3", 300);
    
    // 交换缓存
    swap(cache1, cache2);
    
    // 验证交换结果
    EXPECT_EQ(cache1.size(), 1);
    EXPECT_EQ(cache2.size(), 2);
    
    int value;
    EXPECT_TRUE(cache1.find("key3", value));
    EXPECT_EQ(value, 300);
    
    EXPECT_TRUE(cache2.find("key1", value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(cache2.find("key2", value));
    EXPECT_EQ(value, 200);
}

// 测试重复键的处理
TEST(MemCacheTest, DuplicateKeys) {
    MemCache<std::string, int> cache;
    
    // 插入键值对
    EXPECT_TRUE(cache.insert("key", 100));
    EXPECT_EQ(cache.size(), 1);
    
    // 使用相同的键插入不同的值
    EXPECT_TRUE(cache.insert("key", 200));
    EXPECT_EQ(cache.size(), 1);
    
    // 验证值被覆盖
    int value;
    EXPECT_TRUE(cache.find("key", value));
    EXPECT_EQ(value, 200);
}

// 测试不同类型的缓存键和值
TEST(MemCacheTest, DifferentTypes) {
    // 整数键和字符串值
    {
        MemCache<int, std::string> cache;
        cache.insert(1, "one");
        cache.insert(2, "two");
        
        std::string value;
        EXPECT_TRUE(cache.find(1, value));
        EXPECT_EQ(value, "one");
    }
    
    // 字符串键和布尔值
    {
        MemCache<std::string, bool> cache;
        cache.insert("true", true);
        cache.insert("false", false);
        
        bool value;
        EXPECT_TRUE(cache.find("true", value));
        EXPECT_TRUE(value);
        EXPECT_TRUE(cache.find("false", value));
        EXPECT_FALSE(value);
    }
    
    // 浮点数作为值
    {
        MemCache<std::string, double> cache;
        cache.insert("pi", 3.14159);
        
        double value;
        EXPECT_TRUE(cache.find("pi", value));
        EXPECT_DOUBLE_EQ(value, 3.14159);
    }
} 