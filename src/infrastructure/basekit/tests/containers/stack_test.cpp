#include <gtest/gtest.h>
#include "containers/stack.h"
#include <string>
#include <vector>

using namespace BaseKit;

// 测试节点类
class TestNode
{
public:
    TestNode* next;  // 符合Stack的要求，必须有next指针
    int value;

    TestNode() : next(nullptr), value(0) {}
    explicit TestNode(int val) : next(nullptr), value(val) {}
};

// 测试基本的Stack功能
TEST(StackTest, BasicOperations) {
    // 创建一个空栈
    Stack<TestNode> stack;
    
    // 验证空栈状态
    EXPECT_TRUE(stack.empty());
    EXPECT_FALSE(bool(stack));
    EXPECT_EQ(stack.size(), 0);
    EXPECT_EQ(stack.top(), nullptr);
    
    // 创建测试节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    
    // 插入节点
    stack.push(node1);
    
    // 验证只有一个节点时的状态
    EXPECT_FALSE(stack.empty());
    EXPECT_TRUE(bool(stack));
    EXPECT_EQ(stack.size(), 1);
    EXPECT_EQ(stack.top(), &node1);
    
    // 继续插入节点 - 注意栈是LIFO（后进先出），所以最后插入的在顶部
    stack.push(node2);
    stack.push(node3);
    
    // 验证多个节点时的状态
    EXPECT_EQ(stack.size(), 3);
    EXPECT_EQ(stack.top(), &node3); // 节点3应该在栈顶
    
    // 测试弹出节点
    TestNode* popped = stack.pop();
    EXPECT_EQ(popped, &node3); // 节点3应该被弹出
    EXPECT_EQ(popped->value, 3);
    EXPECT_EQ(stack.size(), 2);
    EXPECT_EQ(stack.top(), &node2); // 现在节点2应该在栈顶
    
    // 继续弹出
    popped = stack.pop();
    EXPECT_EQ(popped, &node2);
    EXPECT_EQ(popped->value, 2);
    EXPECT_EQ(stack.size(), 1);
    EXPECT_EQ(stack.top(), &node1); // 现在节点1应该在栈顶
    
    // 弹出最后一个节点
    popped = stack.pop();
    EXPECT_EQ(popped, &node1);
    EXPECT_EQ(popped->value, 1);
    EXPECT_EQ(stack.size(), 0);
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.top(), nullptr);
    
    // 测试弹出空栈
    popped = stack.pop();
    EXPECT_EQ(popped, nullptr);
}

// 测试清空栈
TEST(StackTest, ClearStack) {
    Stack<TestNode> stack;
    
    // 添加节点
    TestNode node1(1);
    TestNode node2(2);
    stack.push(node1);
    stack.push(node2);
    
    EXPECT_EQ(stack.size(), 2);
    
    // 清空栈
    stack.clear();
    
    // 验证栈状态
    EXPECT_TRUE(stack.empty());
    EXPECT_EQ(stack.size(), 0);
    EXPECT_EQ(stack.top(), nullptr);
}

// 测试栈反转
TEST(StackTest, ReverseStack) {
    Stack<TestNode> stack;
    
    // 添加节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    stack.push(node1); // 底部
    stack.push(node2); // 中间
    stack.push(node3); // 顶部
    
    // 反转栈 - 现在node1应该在顶部，node3在底部
    stack.reverse();
    
    // 验证反转后的顺序
    EXPECT_EQ(stack.size(), 3);
    EXPECT_EQ(stack.top(), &node1); // node1应该在栈顶
    
    TestNode* popped = stack.pop();
    EXPECT_EQ(popped, &node1);
    
    popped = stack.pop();
    EXPECT_EQ(popped, &node2);
    
    popped = stack.pop();
    EXPECT_EQ(popped, &node3);
    
    EXPECT_TRUE(stack.empty());
}

// 测试栈交换
TEST(StackTest, SwapStacks) {
    Stack<TestNode> stack1;
    Stack<TestNode> stack2;
    
    // 准备测试节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    TestNode node4(4);
    
    // 填充栈
    stack1.push(node1);
    stack1.push(node2);
    
    stack2.push(node3);
    stack2.push(node4);
    
    // 交换栈
    stack1.swap(stack2);
    
    // 验证交换结果
    EXPECT_EQ(stack1.size(), 2);
    EXPECT_EQ(stack1.top(), &node4);
    
    EXPECT_EQ(stack2.size(), 2);
    EXPECT_EQ(stack2.top(), &node2);
    
    // 测试全局swap函数
    swap(stack1, stack2);
    
    EXPECT_EQ(stack1.size(), 2);
    EXPECT_EQ(stack1.top(), &node2);
    
    EXPECT_EQ(stack2.size(), 2);
    EXPECT_EQ(stack2.top(), &node4);
}

// 测试迭代器功能
TEST(StackTest, Iterators) {
    Stack<TestNode> stack;
    
    // 添加节点 - 注意栈是LIFO，所以迭代器会以反序遍历
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    stack.push(node1);
    stack.push(node2);
    stack.push(node3);
    
    // 测试迭代器遍历 - 应该是 3, 2, 1 顺序
    std::vector<int> expected = {3, 2, 1};
    int i = 0;
    for (auto& node : stack) {
        EXPECT_EQ(node.value, expected[i++]);
    }
    
    // 测试const迭代器
    const Stack<TestNode>& constStack = stack;
    i = 0;
    for (const auto& node : constStack) {
        EXPECT_EQ(node.value, expected[i++]);
    }
    
    // 测试迭代器比较
    auto it1 = stack.begin();
    auto it2 = stack.begin();
    EXPECT_EQ(it1, it2);
    
    ++it1;
    EXPECT_NE(it1, it2);
    
    // 测试迭代器后缀递增
    it2++;
    EXPECT_EQ(it1, it2);
    
    // 测试迭代器解引用
    it1 = stack.begin();
    EXPECT_EQ((*it1).value, 3);
    EXPECT_EQ(it1->value, 3);
    
    // 测试end迭代器
    auto end = stack.end();
    EXPECT_FALSE(bool(end));
    
    // 测试迭代器交换
    it1 = stack.begin();
    it2 = ++stack.begin();
    swap(it1, it2);
    EXPECT_EQ(it1->value, 2);
    EXPECT_EQ(it2->value, 3);
}

// 测试构造函数
TEST(StackTest, Constructors) {
    // 测试基本构造函数
    Stack<TestNode> stack;
    
    // 准备测试节点
    TestNode node1(1);
    TestNode node2(2);
    TestNode node3(3);
    
    // 添加节点到栈
    stack.push(node1);
    stack.push(node2);
    stack.push(node3);
    
    // 验证栈内容
    EXPECT_EQ(stack.size(), 3);
    EXPECT_EQ(stack.top(), &node3);
    
    // 测试拷贝构造函数
    Stack<TestNode> stackCopy(stack);
    EXPECT_EQ(stackCopy.size(), 3);
    
    // 验证原栈不受影响
    EXPECT_EQ(stack.size(), 3);
    
    // 验证拷贝的栈内容
    TestNode* popped = stackCopy.pop();
    EXPECT_EQ(popped, &node3);
    EXPECT_EQ(popped->value, 3);
    
    popped = stackCopy.pop();
    EXPECT_EQ(popped, &node2);
    EXPECT_EQ(popped->value, 2);
    
    popped = stackCopy.pop();
    EXPECT_EQ(popped, &node1);
    EXPECT_EQ(popped->value, 1);
    
    EXPECT_TRUE(stackCopy.empty());
    
    // 原栈仍然包含所有节点
    EXPECT_EQ(stack.size(), 3);
} 