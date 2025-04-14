#include <gtest/gtest.h>
#include "cache/filecache.h"
#include "filesystem/path.h"
#include "filesystem/file.h"
#include "filesystem/directory.h"
#include "time/timespan.h"
#include <string>
#include <thread>

using namespace BaseKit;

// 测试基本文件缓存操作
TEST(FileCacheTest, BasicOperations) {
    // 创建一个文件缓存实例
    FileCache cache;
    
    // 验证新创建的缓存为空
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0);
    EXPECT_FALSE(bool(cache));
    
    // 插入值到缓存
    EXPECT_TRUE(cache.insert("key1", "value1"));
    EXPECT_TRUE(cache.insert("key2", "value2"));
    
    // 验证缓存不再为空
    EXPECT_FALSE(cache.empty());
    EXPECT_EQ(cache.size(), 2);
    EXPECT_TRUE(bool(cache));
    
    // 查找缓存中的值
    auto [found1, value1] = cache.find("key1");
    EXPECT_TRUE(found1);
    EXPECT_EQ(value1, "value1");
    
    auto [found2, value2] = cache.find("key2");
    EXPECT_TRUE(found2);
    EXPECT_EQ(value2, "value2");
    
    auto [found3, value3] = cache.find("key3");
    EXPECT_FALSE(found3);
    
    // 移除缓存中的值
    EXPECT_TRUE(cache.remove("key1"));
    auto [found1_after, value1_after] = cache.find("key1");
    EXPECT_FALSE(found1_after);
    auto [found2_after, value2_after] = cache.find("key2");
    EXPECT_TRUE(found2_after);
    EXPECT_EQ(cache.size(), 1);
    
    // 清空缓存
    cache.clear();
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.size(), 0);
}

// 测试emplace操作
TEST(FileCacheTest, EmplaceOperation) {
    FileCache cache;
    
    // 使用emplace添加值
    EXPECT_TRUE(cache.emplace("key1", "value1"));
    EXPECT_TRUE(cache.emplace("key2", "value2"));
    
    // 验证值正确存储
    auto [found1, value1] = cache.find("key1");
    EXPECT_TRUE(found1);
    EXPECT_EQ(value1, "value1");
    
    auto [found2, value2] = cache.find("key2");
    EXPECT_TRUE(found2);
    EXPECT_EQ(value2, "value2");
}

// 测试超时功能
TEST(FileCacheTest, TimeoutFeature) {
    FileCache cache;
    
    // 添加有超时的值
    EXPECT_TRUE(cache.insert("key1", "value1", Timespan::milliseconds(100))); // 100毫秒后过期
    EXPECT_TRUE(cache.insert("key2", "value2", Timespan::seconds(10))); // 10秒后过期
    
    // 验证值存在
    auto [found1, _] = cache.find("key1");
    EXPECT_TRUE(found1);
    auto [found2, __] = cache.find("key2");
    EXPECT_TRUE(found2);
    
    // 等待第一个值过期
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // 触发watchdog机制
    cache.watchdog();
    
    // 验证第一个值已过期，第二个值仍然存在
    auto [found1_after, ___] = cache.find("key1");
    EXPECT_FALSE(found1_after);
    auto [found2_after, ____] = cache.find("key2");
    EXPECT_TRUE(found2_after);
    
    // 验证缓存大小
    EXPECT_EQ(cache.size(), 1);
}

// 测试超时值查询
TEST(FileCacheTest, TimeoutQuery) {
    FileCache cache;
    
    // 添加具有不同超时的值
    EXPECT_TRUE(cache.insert("key1", "value1", Timespan::seconds(5)));
    
    // 查询值和其超时时间
    Timestamp timeout;
    auto [found, value] = cache.find("key1", timeout);
    EXPECT_TRUE(found);
    EXPECT_EQ(value, "value1");
    
    // 验证超时时间在将来
    EXPECT_GT(timeout, UtcTimestamp());
}

// 测试文件路径操作
TEST(FileCacheTest, PathOperations) {
    FileCache cache;
    Path testFile = Path::temp() / "test_file.txt";
    
    // 确保测试文件存在
    {
        File file(testFile);
        file.Open(true, true, true); // read=true, write=true, truncate=true
        file.Write("Test content");
        file.Close();
    }
    
    // 插入文件路径到缓存
    EXPECT_TRUE(cache.insert_path(testFile, "/test/"));
    
    // 查找路径
    EXPECT_TRUE(cache.find_path(testFile));
    
    // 查找路径和超时信息
    Timestamp timeout;
    EXPECT_TRUE(cache.find_path(testFile, timeout));
    
    // 移除路径
    EXPECT_TRUE(cache.remove_path(testFile));
    EXPECT_FALSE(cache.find_path(testFile));
    
    // 删除测试文件
    File::Remove(testFile);
}

// 测试缓存交换
TEST(FileCacheTest, SwapOperation) {
    FileCache cache1;
    FileCache cache2;
    
    // 填充第一个缓存
    cache1.insert("key1", "value1");
    cache1.insert("key2", "value2");
    
    // 填充第二个缓存
    cache2.insert("key3", "value3");
    
    // 交换缓存
    swap(cache1, cache2);
    
    // 验证交换结果
    EXPECT_EQ(cache1.size(), 1);
    EXPECT_EQ(cache2.size(), 2);
    
    auto [found1, value1] = cache1.find("key3");
    EXPECT_TRUE(found1);
    EXPECT_EQ(value1, "value3");
    
    auto [found2a, value2a] = cache2.find("key1");
    EXPECT_TRUE(found2a);
    EXPECT_EQ(value2a, "value1");
    
    auto [found2b, value2b] = cache2.find("key2");
    EXPECT_TRUE(found2b);
    EXPECT_EQ(value2b, "value2");
}

// 测试重复键的处理
TEST(FileCacheTest, DuplicateKeys) {
    FileCache cache;
    
    // 插入键值对
    EXPECT_TRUE(cache.insert("key", "value1"));
    EXPECT_EQ(cache.size(), 1);
    
    // 使用相同的键插入不同的值
    EXPECT_TRUE(cache.insert("key", "value2"));
    EXPECT_EQ(cache.size(), 1);
    
    // 验证值被覆盖
    auto [found, value] = cache.find("key");
    EXPECT_TRUE(found);
    EXPECT_EQ(value, "value2");
}

// 测试自定义路径处理器
TEST(FileCacheTest, CustomPathHandler) {
    FileCache cache;
    Path testDir = Path::temp() / "test_dir";
    
    // 确保测试目录存在
    Directory::Create(testDir);
    
    // 创建测试文件
    Path testFile1 = testDir / "file1.txt";
    Path testFile2 = testDir / "file2.txt";
    
    {
        File file1(testFile1);
        file1.Open(true, true, true); // read=true, write=true, truncate=true
        file1.Write("Content 1");
        file1.Close();
        
        File file2(testFile2);
        file2.Open(true, true, true); // read=true, write=true, truncate=true
        file2.Write("Content 2");
        file2.Close();
    }
    
    // 自定义处理器，将文件内容转换为大写
    auto customHandler = [](FileCache& cache, const std::string& key, const std::string& value, const Timespan& timeout) -> bool {
        std::string upperValue;
        for (char c : value) {
            upperValue += std::toupper(c);
        }
        return cache.insert(key, upperValue, timeout);
    };
    
    // 插入目录，使用自定义处理器
    EXPECT_TRUE(cache.insert_path(testDir, "/test/", Timespan(0), customHandler));
    
    // 查找处理后的文件内容
    auto [found1, value1] = cache.find("/test/file1.txt");
    EXPECT_TRUE(found1);
    EXPECT_EQ(value1, "CONTENT 1");
    
    auto [found2, value2] = cache.find("/test/file2.txt");
    EXPECT_TRUE(found2);
    EXPECT_EQ(value2, "CONTENT 2");
    
    // 清理
    File::Remove(testFile1);
    File::Remove(testFile2);
    Directory::Remove(testDir);
} 