#include <gtest/gtest.h>
#include "utility/singleton.h"
#include <string>

using namespace BaseKit;

// 定义测试用的单例类
class TestSingleton : public Singleton<TestSingleton>
{
    friend class Singleton<TestSingleton>;
    
private:
    TestSingleton() : value(0) {}

public:
    int value;
    std::string name;
    
    void setValue(int val) { value = val; }
    int getValue() const { return value; }
    
    void setName(const std::string& newName) { name = newName; }
    std::string getName() const { return name; }
};

// 定义另一个测试用的单例类
class AnotherSingleton : public Singleton<AnotherSingleton>
{
    friend class Singleton<AnotherSingleton>;
    
private:
    AnotherSingleton() : counter(0) {}

public:
    int counter;
    
    void increment() { counter++; }
    int getCounter() const { return counter; }
};

// 测试单例类基本功能
TEST(SingletonTest, BasicFunctionality) {
    // 获取单例实例
    TestSingleton& instance = TestSingleton::GetInstance();
    
    // 设置初始值
    instance.setValue(42);
    instance.setName("TestInstance");
    
    // 确认值已正确设置
    EXPECT_EQ(instance.getValue(), 42);
    EXPECT_EQ(instance.getName(), "TestInstance");
    
    // 通过另一个引用修改值
    TestSingleton& sameInstance = TestSingleton::GetInstance();
    sameInstance.setValue(100);
    sameInstance.setName("ModifiedName");
    
    // 验证原实例的值也被修改（因为是同一个对象）
    EXPECT_EQ(instance.getValue(), 100);
    EXPECT_EQ(instance.getName(), "ModifiedName");
    
    // 确认两个引用指向同一个实例
    EXPECT_EQ(&instance, &sameInstance);
}

// 测试多个不同类型的单例
TEST(SingletonTest, MultipleSingletons) {
    // 获取第一个单例类型的实例
    TestSingleton& instance1 = TestSingleton::GetInstance();
    instance1.setValue(42);
    
    // 获取第二个单例类型的实例
    AnotherSingleton& instance2 = AnotherSingleton::GetInstance();
    instance2.increment();
    instance2.increment();
    
    // 验证各自的值
    EXPECT_EQ(instance1.getValue(), 42);
    EXPECT_EQ(instance2.getCounter(), 2);
    
    // 确认不同类型的单例是不同的实例
    EXPECT_NE((void*)&instance1, (void*)&instance2);
}

// 测试线程安全性（间接测试，通过多次获取实例）
TEST(SingletonTest, ThreadSafety) {
    // 在同一个线程中多次获取实例，确保都是同一个对象
    TestSingleton& instance1 = TestSingleton::GetInstance();
    TestSingleton& instance2 = TestSingleton::GetInstance();
    TestSingleton& instance3 = TestSingleton::GetInstance();
    
    // 设置一个实例的值
    instance1.setValue(123);
    
    // 验证所有实例的值都被更新（因为都是同一个对象）
    EXPECT_EQ(instance2.getValue(), 123);
    EXPECT_EQ(instance3.getValue(), 123);
    
    // 验证所有实例都是同一个对象
    EXPECT_EQ(&instance1, &instance2);
    EXPECT_EQ(&instance2, &instance3);
}

// 自定义单例销毁顺序测试类
class DestructionTracker
{
public:
    static bool instanceDestroyed;
    
    DestructionTracker() {
        instanceDestroyed = false;
    }
    
    ~DestructionTracker() {
        instanceDestroyed = true;
    }
};

bool DestructionTracker::instanceDestroyed = false;

// 使用DestructionTracker的单例类
class TrackedSingleton : public Singleton<TrackedSingleton>
{
    friend class Singleton<TrackedSingleton>;
    
private:
    TrackedSingleton() : tracker() {}
    DestructionTracker tracker;
};

// 注意：这个测试主要是为了验证单例的创建，
// 但无法在测试框架中测试程序结束时的析构
TEST(SingletonTest, Creation) {
    // 获取跟踪销毁的单例实例
    TrackedSingleton& instance = TrackedSingleton::GetInstance();
    
    // 使用 instance 引用以避免未使用变量警告
    EXPECT_TRUE(&instance != nullptr);
    
    // 验证实例存在且销毁标志为false
    EXPECT_FALSE(DestructionTracker::instanceDestroyed);
    
    // 注意：无法在测试框架中验证程序结束时的析构
} 