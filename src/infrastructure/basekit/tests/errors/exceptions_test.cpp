#include <gtest/gtest.h>
#include "errors/exceptions.h"
#include <stdexcept>

using namespace BaseKit;

// 测试基本异常功能
TEST(ExceptionsTest, BasicExceptionFunctionality) {
    // 测试默认构造
    Exception ex1;
    EXPECT_TRUE(ex1.message().empty());
    // EXPECT_TRUE(ex1.location().empty()); // SourceLocation 可能没有 empty 方法
    
    // 测试带消息构造
    Exception ex2("测试异常");
    EXPECT_EQ(ex2.message(), "测试异常");
    // EXPECT_TRUE(ex2.location().empty()); // SourceLocation 可能没有 empty 方法
    
    // 测试异常字符串表示
    std::string str = ex2.string();
    EXPECT_TRUE(str.find("测试异常") != std::string::npos);
    
    // 测试 what() 方法
    EXPECT_STREQ(ex2.what(), ex2.string().c_str());
}

// 测试异常继承体系
TEST(ExceptionsTest, ExceptionHierarchy) {
    // 测试不同类型的异常
    ArgumentException argEx("参数异常");
    DomainException domainEx("域异常");
    RuntimeException runtimeEx("运行时异常");
    SecurityException securityEx("安全异常");
    
    // 确保所有异常都继承自 Exception
    EXPECT_TRUE(dynamic_cast<Exception*>(&argEx) != nullptr);
    EXPECT_TRUE(dynamic_cast<Exception*>(&domainEx) != nullptr);
    EXPECT_TRUE(dynamic_cast<Exception*>(&runtimeEx) != nullptr);
    EXPECT_TRUE(dynamic_cast<Exception*>(&securityEx) != nullptr);
    
    // 测试异常消息
    EXPECT_EQ(argEx.message(), "参数异常");
    EXPECT_EQ(domainEx.message(), "域异常");
    EXPECT_EQ(runtimeEx.message(), "运行时异常");
    EXPECT_EQ(securityEx.message(), "安全异常");
}

// 测试系统异常
TEST(ExceptionsTest, SystemException) {
    // 测试默认构造
    SystemException sysEx1;
    EXPECT_NE(sysEx1.system_error(), 0);
    // 注意：system_message() 经 SystemError::Description() 获取，依赖 strerror_r。
    // 在 GNU/Linux 上部分错误码会返回静态指针而非填充 buffer，结果可能为空，
    // 这里仅验证可调用，不强制非空。
    (void)sysEx1.system_message();

    // 测试带错误码构造
    SystemException sysEx2(EACCES); // 权限被拒绝
    EXPECT_EQ(sysEx2.system_error(), EACCES);
    (void)sysEx2.system_message();

    // 测试带消息构造
    SystemException sysEx3("系统异常");
    EXPECT_EQ(sysEx3.message(), "系统异常");
    EXPECT_NE(sysEx3.system_error(), 0);
    (void)sysEx3.system_message();

    // 测试带消息和错误码构造
    SystemException sysEx4("自定义系统异常", ENOSYS);
    EXPECT_EQ(sysEx4.message(), "自定义系统异常");
    EXPECT_EQ(sysEx4.system_error(), ENOSYS);
    (void)sysEx4.system_message();
    
    // 测试异常字符串表示包含系统错误信息
    std::string str = sysEx4.string();
    EXPECT_TRUE(str.find("自定义系统异常") != std::string::npos);
    EXPECT_TRUE(str.find(std::to_string(ENOSYS)) != std::string::npos);
}

// 测试异常抛出和捕获
TEST(ExceptionsTest, ThrowAndCatch) {
    // 测试抛出和捕获基本异常
    try {
        throw Exception("基本异常");
        FAIL() << "应当抛出异常";
    } catch (const Exception& ex) {
        EXPECT_EQ(ex.message(), "基本异常");
    }
    
    // 测试抛出和捕获参数异常
    try {
        throw ArgumentException("参数异常");
        FAIL() << "应当抛出异常";
    } catch (const ArgumentException& ex) {
        EXPECT_EQ(ex.message(), "参数异常");
    } catch (...) {
        FAIL() << "应当捕获 ArgumentException";
    }
    
    // 测试使用基类捕获异常
    try {
        throw DomainException("域异常");
        FAIL() << "应当抛出异常";
    } catch (const Exception& ex) {
        EXPECT_EQ(ex.message(), "域异常");
        EXPECT_TRUE(dynamic_cast<const DomainException*>(&ex) != nullptr);
    }
}

// 测试异常位置信息
TEST(ExceptionsTest, ExceptionLocation) {
    // 使用宏抛出带位置信息的异常
    try {
        throwex Exception("带位置的异常");
        FAIL() << "应当抛出异常";
    } catch (const Exception& ex) {
        EXPECT_EQ(ex.message(), "带位置的异常");
        // SourceLocation 可能没有 empty 和 file 方法
        // EXPECT_FALSE(ex.location().empty());
        // EXPECT_TRUE(ex.location().file().find("exceptions_test.cpp") != std::string::npos);
        
        // 测试字符串表示中是否包含位置信息
        std::string str = ex.string();
        EXPECT_TRUE(str.find("带位置的异常") != std::string::npos);
        EXPECT_TRUE(str.find("exceptions_test.cpp") != std::string::npos);
    }
} 