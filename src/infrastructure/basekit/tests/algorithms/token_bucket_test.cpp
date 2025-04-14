#include <gtest/gtest.h>
#include "algorithms/token_bucket.h"
#include "time/timestamp.h"
#include <thread>
#include <chrono>

using namespace BaseKit;

// 测试TokenBucket基本功能
TEST(TokenBucketTest, BasicFunctionality) {
    // 创建一个每秒10个令牌，最大容量为20个令牌的桶
    TokenBucket bucket(10, 20);
    
    // 测试消费单个令牌
    EXPECT_TRUE(bucket.Consume(1));
    
    // 测试消费多个令牌
    EXPECT_TRUE(bucket.Consume(5));
    
    // 测试拷贝构造函数
    TokenBucket bucket2(bucket);
    EXPECT_TRUE(bucket2.Consume(1));
    
    // 测试赋值操作符
    TokenBucket bucket3(100, 100);
    bucket3 = bucket;
    EXPECT_TRUE(bucket3.Consume(1));
}

// 测试TokenBucket速率限制功能
TEST(TokenBucketTest, RateLimit) {
    // 创建一个每秒10个令牌，最大容量为10个令牌的桶
    TokenBucket bucket(10, 10);
    
    // 立即消费所有令牌（最大容量）
    EXPECT_TRUE(bucket.Consume(10));
    
    // 由于令牌已耗尽，此时无法继续消费
    EXPECT_FALSE(bucket.Consume(1));
    
    // 等待一小段时间，让桶重新积累一些令牌
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 等待200毫秒，应该积累约2个令牌
    
    // 尝试消费1个令牌，应该成功
    EXPECT_TRUE(bucket.Consume(1));
    
    // 尝试消费2个令牌，应该失败（因为只积累了约1个令牌）
    EXPECT_FALSE(bucket.Consume(2));
    
    // 再等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(300)); // 再等待300毫秒，应该再积累约3个令牌
    
    // 尝试消费3个令牌，应该成功
    EXPECT_TRUE(bucket.Consume(3));
}

// 测试TokenBucket突发流量控制
TEST(TokenBucketTest, BurstControl) {
    // 创建一个每秒20个令牌，最大容量为100个令牌的桶（允许短时间内的突发流量）
    TokenBucket bucket(20, 100);
    
    // 初始桶是满的，不需要时间积累
    // 在初始状态下应该有100个令牌
    EXPECT_TRUE(bucket.Consume(100));
    
    // 尝试立即消费更多令牌，应该失败
    EXPECT_FALSE(bucket.Consume(1));
    
    // 等待足够长的时间，让桶积累一些令牌
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 等待500毫秒，应该积累约10个令牌
    
    // 测试积累的令牌数
    EXPECT_TRUE(bucket.Consume(10)); // 应该成功消费10个令牌
    EXPECT_FALSE(bucket.Consume(1)); // 再消费应该失败
    
    // 再等待一段时间，让桶积累更多令牌
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待1秒，应该积累约20个令牌
    
    // 测试积累的令牌数
    EXPECT_TRUE(bucket.Consume(20)); // 应该成功消费20个令牌
    EXPECT_FALSE(bucket.Consume(1)); // 再消费应该失败
}

// 测试TokenBucket的长时间不使用后的行为
TEST(TokenBucketTest, LongInactivity) {
    // 创建一个每秒5个令牌，最大容量为10个令牌的桶
    TokenBucket bucket(5, 10);
    
    // 等待足够长的时间，让桶积累满令牌
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 等待3秒，应该积累约10个令牌（最大容量）
    
    // 测试桶是否已经填满
    EXPECT_TRUE(bucket.Consume(10)); // 应该能消费10个令牌
    EXPECT_FALSE(bucket.Consume(1)); // 桶应该为空
    
    // 测试长时间不使用后再次使用
    std::this_thread::sleep_for(std::chrono::seconds(10)); // 等待10秒
    
    // 测试是否能够消费最大容量的令牌（桶应该已重新填满）
    EXPECT_TRUE(bucket.Consume(10));
}

// 测试极限情况
TEST(TokenBucketTest, EdgeCases) {
    // 测试极低速率的桶
    TokenBucket lowRate(1, 1); // 每秒只有1个令牌
    EXPECT_TRUE(lowRate.Consume(1)); // 初始应该有1个令牌
    EXPECT_FALSE(lowRate.Consume(1)); // 现在应该没有令牌了
    
    // 测试极高速率的桶
    TokenBucket highRate(1000000, 1000); // 每秒100万个令牌
    EXPECT_TRUE(highRate.Consume(1000)); // 应该能立即消费1000个令牌
    
    // 测试0令牌消费
    TokenBucket normal(10, 10);
    EXPECT_TRUE(normal.Consume(0)); // 消费0个令牌应该总是成功
} 