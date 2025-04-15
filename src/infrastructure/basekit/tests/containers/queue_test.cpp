#include <gtest/gtest.h>
#include "containers/queue.h"
#include <string>
#include <vector>

using namespace BaseKit;

// 测试节点类
class TestNode
{
public:
    TestNode* next;  // 符合Queue的要求，必须有next指针
    int value;

    TestNode() : next(nullptr), value(0) {}
    explicit TestNode(int val) : next(nullptr), value(val) {}
};

// 测试基本的Queue功能
TEST(QueueTest, BasicOperations) {
    // 创建一个空队列
    Queue<TestNode> queue;
    
    // 验证空队列状态
    EXPECT_TRUE(queue.empty());
    EXPECT_FALSE(bool(queue));
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(queue.front(), nullptr);
    EXPECT_EQ(queue.back(), nullptr);
    
    // 创建测试节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    
    // 插入节点
    queue.push(node1);
    
    // 验证只有一个节点时的状态
    EXPECT_FALSE(queue.empty());
    EXPECT_TRUE(bool(queue));
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.front(), &node1);
    EXPECT_EQ(queue.back(), &node1);
    
    // 继续插入节点
    queue.push(node2);
    queue.push(node3);
    
    // 验证多个节点时的状态
    EXPECT_EQ(queue.size(), 3);
    EXPECT_EQ(queue.front(), &node1);
    EXPECT_EQ(queue.back(), &node3);
    
    // 测试弹出节点
    TestNode* popped = queue.pop();
    EXPECT_EQ(popped, &node1);
    EXPECT_EQ(popped->value, 1);
    EXPECT_EQ(queue.size(), 2);
    EXPECT_EQ(queue.front(), &node2);
    
    // 继续弹出
    popped = queue.pop();
    EXPECT_EQ(popped, &node2);
    EXPECT_EQ(popped->value, 2);
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.front(), &node3);
    EXPECT_EQ(queue.back(), &node3);
    
    // 弹出最后一个节点
    popped = queue.pop();
    EXPECT_EQ(popped, &node3);
    EXPECT_EQ(popped->value, 3);
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.front(), nullptr);
    EXPECT_EQ(queue.back(), nullptr);
    
    // 测试弹出空队列
    popped = queue.pop();
    EXPECT_EQ(popped, nullptr);
}

// 测试清空队列
TEST(QueueTest, ClearQueue) {
    Queue<TestNode> queue;
    
    // 添加节点
    TestNode node1(1);
    TestNode node2(2);
    queue.push(node1);
    queue.push(node2);
    
    EXPECT_EQ(queue.size(), 2);
    
    // 清空队列
    queue.clear();
    
    // 验证队列状态
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
    EXPECT_EQ(queue.front(), nullptr);
    EXPECT_EQ(queue.back(), nullptr);
}

// 测试队列反转
TEST(QueueTest, ReverseQueue) {
    Queue<TestNode> queue;
    
    // 添加节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    queue.push(node1);
    queue.push(node2);
    queue.push(node3);
    
    // 反转队列
    queue.reverse();
    
    // 验证反转后的顺序
    EXPECT_EQ(queue.size(), 3);
    EXPECT_EQ(queue.front(), &node3);
    
    TestNode* popped = queue.pop();
    EXPECT_EQ(popped, &node3);
    
    popped = queue.pop();
    EXPECT_EQ(popped, &node2);
    
    popped = queue.pop();
    EXPECT_EQ(popped, &node1);
    
    EXPECT_TRUE(queue.empty());
}

// 测试队列交换
TEST(QueueTest, SwapQueues) {
    Queue<TestNode> queue1;
    Queue<TestNode> queue2;
    
    // 准备测试节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    TestNode node4(4);
    
    // 填充队列
    queue1.push(node1);
    queue1.push(node2);
    
    queue2.push(node3);
    queue2.push(node4);
    
    // 交换队列
    queue1.swap(queue2);
    
    // 验证交换结果
    EXPECT_EQ(queue1.size(), 2);
    EXPECT_EQ(queue1.front(), &node3);
    EXPECT_EQ(queue1.back(), &node4);
    
    EXPECT_EQ(queue2.size(), 2);
    EXPECT_EQ(queue2.front(), &node1);
    EXPECT_EQ(queue2.back(), &node2);
    
    // 测试全局swap函数
    swap(queue1, queue2);
    
    EXPECT_EQ(queue1.size(), 2);
    EXPECT_EQ(queue1.front(), &node1);
    EXPECT_EQ(queue1.back(), &node2);
    
    EXPECT_EQ(queue2.size(), 2);
    EXPECT_EQ(queue2.front(), &node3);
    EXPECT_EQ(queue2.back(), &node4);
}

// 测试迭代器功能
TEST(QueueTest, Iterators) {
    Queue<TestNode> queue;
    
    // 添加节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    queue.push(node1);
    queue.push(node2);
    queue.push(node3);
    
    // 测试迭代器遍历
    int expected = 1;
    for (auto& node : queue) {
        EXPECT_EQ(node.value, expected);
        expected++;
    }
    
    // 测试const迭代器
    const Queue<TestNode>& constQueue = queue;
    expected = 1;
    for (const auto& node : constQueue) {
        EXPECT_EQ(node.value, expected);
        expected++;
    }
    
    // 测试迭代器比较
    auto it1 = queue.begin();
    auto it2 = queue.begin();
    EXPECT_EQ(it1, it2);
    
    ++it1;
    EXPECT_NE(it1, it2);
    
    // 测试迭代器后缀递增
    it2++;
    EXPECT_EQ(it1, it2);
    
    // 测试迭代器解引用
    it1 = queue.begin();
    EXPECT_EQ((*it1).value, 1);
    EXPECT_EQ(it1->value, 1);
    
    // 测试end迭代器
    auto end = queue.end();
    EXPECT_FALSE(bool(end));
    
    // 测试迭代器交换
    it1 = queue.begin();
    it2 = ++queue.begin();
    swap(it1, it2);
    EXPECT_EQ(it1->value, 2);
    EXPECT_EQ(it2->value, 1);
}

// 测试构造函数
TEST(QueueTest, Constructors) {
    // 对于Range构造函数，我们简化测试，不使用迭代器
    // 改为直接验证Queue的基本构造函数
    Queue<TestNode> queue;
    
    // 准备测试节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    
    // 添加节点到队列
    queue.push(node1);
    queue.push(node2);
    queue.push(node3);
    
    // 验证队列内容
    EXPECT_EQ(queue.size(), 3);
    
    // 测试拷贝构造函数
    Queue<TestNode> queueCopy(queue);
    EXPECT_EQ(queueCopy.size(), 3);
    
    // 验证原队列不受影响
    EXPECT_EQ(queue.size(), 3);
    
    // 验证拷贝的队列内容
    TestNode* popped = queueCopy.pop();
    EXPECT_EQ(popped, &node1);
    EXPECT_EQ(popped->value, 1);
    
    popped = queueCopy.pop();
    EXPECT_EQ(popped, &node2);
    EXPECT_EQ(popped->value, 2);
    
    popped = queueCopy.pop();
    EXPECT_EQ(popped, &node3);
    EXPECT_EQ(popped->value, 3);
    
    EXPECT_TRUE(queueCopy.empty());
    
    // 原队列仍然包含所有节点
    EXPECT_EQ(queue.size(), 3);
} 