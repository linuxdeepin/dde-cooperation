#include <gtest/gtest.h>
#include <QVariant>
#include "message_p.h"
#include "slotipc/argdefine.h"

class MessageTest : public ::testing::Test {
protected:
    // 基本测试设置
};

TEST_F(MessageTest, MessageConstruction) {
    // 测试基本消息构造
    SlotIPCMessage msg(SlotIPCMessage::MessageCallWithReturn, "test.method");
    EXPECT_EQ(msg.messageType(), SlotIPCMessage::MessageCallWithReturn);
    EXPECT_EQ(msg.method(), "test.method");
}

TEST_F(MessageTest, MessageTypes) {
    // 测试不同类型的消息
    SlotIPCMessage callMsg(SlotIPCMessage::MessageCallWithReturn, "test.call");
    EXPECT_EQ(callMsg.messageType(), SlotIPCMessage::MessageCallWithReturn);
    
    SlotIPCMessage responseMsg(SlotIPCMessage::MessageResponse, "test.response");
    EXPECT_EQ(responseMsg.messageType(), SlotIPCMessage::MessageResponse);
    
    SlotIPCMessage errorMsg(SlotIPCMessage::MessageError, "test.error");
    EXPECT_EQ(errorMsg.messageType(), SlotIPCMessage::MessageError);
    
    SlotIPCMessage signalMsg(SlotIPCMessage::MessageSignal, "test.signal");
    EXPECT_EQ(signalMsg.messageType(), SlotIPCMessage::MessageSignal);
}

TEST_F(MessageTest, ReturnType) {
    // 测试返回类型设置
    SlotIPCMessage msg(SlotIPCMessage::MessageCallWithReturn, "test.method", 
                      METHOD_ARG(), METHOD_ARG(), METHOD_ARG(),
                      METHOD_ARG(), METHOD_ARG(), METHOD_ARG(),
                      METHOD_ARG(), METHOD_ARG(), METHOD_ARG(),
                      METHOD_ARG(), "QString");
    EXPECT_EQ(msg.returnType(), "QString");
}