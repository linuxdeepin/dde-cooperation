#include <gtest/gtest.h>
#include "../src/marshaller_p.h"
#include "../src/message_p.h"

class MarshallerTest : public ::testing::Test {
protected:
    SlotIPCMarshaller marshaller;
};

TEST_F(MarshallerTest, MessageMarshalling) {
    // 测试消息编解码
    SlotIPCMessage::Arguments args;
    SlotIPCMessage message(SlotIPCMessage::MessageCallWithReturn, 
                          "testMethod", 
                          args,
                          "int");
    
    QByteArray data = SlotIPCMarshaller::marshallMessage(message);
    SlotIPCMessage result = SlotIPCMarshaller::demarshallMessage(data);
    
    EXPECT_EQ(result.method(), "testMethod");
    EXPECT_EQ(result.arguments().size(), 0);
}

TEST_F(MarshallerTest, MessageType) {
    // 测试消息类型
    SlotIPCMessage::Arguments args;
    SlotIPCMessage message(SlotIPCMessage::MessageSignal, 
                          "testSignal",
                          args);
    
    QByteArray data = SlotIPCMarshaller::marshallMessage(message);
    SlotIPCMessage::MessageType type = SlotIPCMarshaller::demarshallMessageType(data);
    
    EXPECT_EQ(type, SlotIPCMessage::MessageSignal);
}