#include <gtest/gtest.h>
#include "utility/endian.h"
#include <limits>
#include <array>

using namespace BaseKit;

// 测试端序检测功能
TEST(EndianTest, DetectionFunctions) {
    // 检测主机端序
    EXPECT_TRUE(Endian::IsLittleEndian() || Endian::IsBigEndian());
    EXPECT_NE(Endian::IsLittleEndian(), Endian::IsBigEndian());
}

// 测试读写16位整数
TEST(EndianTest, ReadWriteInt16) {
    // 创建测试值和缓冲区
    int16_t originalValue = 0x1234;
    uint8_t buffer[2];
    
    // 写入大端序
    Endian::WriteBigEndian(buffer, originalValue);
    
    // 验证大端序格式（高位在前）
    EXPECT_EQ(buffer[0], 0x12);
    EXPECT_EQ(buffer[1], 0x34);
    
    // 读取大端序
    int16_t readValue = 0;
    Endian::ReadBigEndian(buffer, readValue);
    EXPECT_EQ(readValue, originalValue);
    
    // 写入小端序
    Endian::WriteLittleEndian(buffer, originalValue);
    
    // 验证小端序格式（低位在前）
    EXPECT_EQ(buffer[0], 0x34);
    EXPECT_EQ(buffer[1], 0x12);
    
    // 读取小端序
    readValue = 0;
    Endian::ReadLittleEndian(buffer, readValue);
    EXPECT_EQ(readValue, originalValue);
}

// 测试32位整数
TEST(EndianTest, ReadWriteInt32) {
    // 创建测试值和缓冲区
    int32_t originalValue = 0x12345678;
    uint8_t buffer[4];
    
    // 写入大端序
    Endian::WriteBigEndian(buffer, originalValue);
    
    // 验证大端序格式
    EXPECT_EQ(buffer[0], 0x12);
    EXPECT_EQ(buffer[1], 0x34);
    EXPECT_EQ(buffer[2], 0x56);
    EXPECT_EQ(buffer[3], 0x78);
    
    // 读取大端序
    int32_t readValue = 0;
    Endian::ReadBigEndian(buffer, readValue);
    EXPECT_EQ(readValue, originalValue);
    
    // 写入小端序
    Endian::WriteLittleEndian(buffer, originalValue);
    
    // 验证小端序格式
    EXPECT_EQ(buffer[0], 0x78);
    EXPECT_EQ(buffer[1], 0x56);
    EXPECT_EQ(buffer[2], 0x34);
    EXPECT_EQ(buffer[3], 0x12);
    
    // 读取小端序
    readValue = 0;
    Endian::ReadLittleEndian(buffer, readValue);
    EXPECT_EQ(readValue, originalValue);
}

// 测试64位整数
TEST(EndianTest, ReadWriteInt64) {
    // 创建测试值和缓冲区
    int64_t originalValue = 0x1234567890ABCDEF;
    uint8_t buffer[8];
    
    // 写入大端序
    Endian::WriteBigEndian(buffer, originalValue);
    
    // 验证大端序格式
    EXPECT_EQ(buffer[0], 0x12);
    EXPECT_EQ(buffer[1], 0x34);
    EXPECT_EQ(buffer[2], 0x56);
    EXPECT_EQ(buffer[3], 0x78);
    EXPECT_EQ(buffer[4], 0x90);
    EXPECT_EQ(buffer[5], 0xAB);
    EXPECT_EQ(buffer[6], 0xCD);
    EXPECT_EQ(buffer[7], 0xEF);
    
    // 读取大端序
    int64_t readValue = 0;
    Endian::ReadBigEndian(buffer, readValue);
    EXPECT_EQ(readValue, originalValue);
    
    // 写入小端序
    Endian::WriteLittleEndian(buffer, originalValue);
    
    // 验证小端序格式
    EXPECT_EQ(buffer[0], 0xEF);
    EXPECT_EQ(buffer[1], 0xCD);
    EXPECT_EQ(buffer[2], 0xAB);
    EXPECT_EQ(buffer[3], 0x90);
    EXPECT_EQ(buffer[4], 0x78);
    EXPECT_EQ(buffer[5], 0x56);
    EXPECT_EQ(buffer[6], 0x34);
    EXPECT_EQ(buffer[7], 0x12);
    
    // 读取小端序
    readValue = 0;
    Endian::ReadLittleEndian(buffer, readValue);
    EXPECT_EQ(readValue, originalValue);
}

// 测试浮点数转换
TEST(EndianTest, FloatConversion) {
    // 创建测试值
    const float originalValue = 123.456f;
    uint8_t buffer[4];
    
    // 将浮点数转为字节数组
    memcpy(buffer, &originalValue, sizeof(float));
    
    // 创建大端序缓冲区
    uint8_t bigEndianBuffer[4];
    if (Endian::IsLittleEndian()) {
        // 如果是小端序系统，反转字节顺序
        bigEndianBuffer[0] = buffer[3];
        bigEndianBuffer[1] = buffer[2];
        bigEndianBuffer[2] = buffer[1];
        bigEndianBuffer[3] = buffer[0];
    } else {
        // 如果是大端序系统，保持字节顺序
        memcpy(bigEndianBuffer, buffer, 4);
    }
    
    // 从大端序缓冲区读取浮点数
    float resultValue;
    uint32_t temp;
    Endian::ReadBigEndian(bigEndianBuffer, temp);
    memcpy(&resultValue, &temp, sizeof(float));
    
    // 验证转换后的值是否相同
    EXPECT_FLOAT_EQ(resultValue, originalValue);
}

// 测试无符号整数类型转换
TEST(EndianTest, UnsignedTypes) {
    // 创建测试值
    const uint16_t uint16Value = 0xABCD;
    const uint32_t uint32Value = 0xABCDEF01;
    const uint64_t uint64Value = 0xABCDEF0123456789;
    
    // 测试缓冲区
    uint8_t buffer16[2], buffer32[4], buffer64[8];
    
    // 写入大端序
    Endian::WriteBigEndian(buffer16, uint16Value);
    Endian::WriteBigEndian(buffer32, uint32Value);
    Endian::WriteBigEndian(buffer64, uint64Value);
    
    // 验证大端序格式
    EXPECT_EQ(buffer16[0], 0xAB);
    EXPECT_EQ(buffer16[1], 0xCD);
    
    EXPECT_EQ(buffer32[0], 0xAB);
    EXPECT_EQ(buffer32[1], 0xCD);
    EXPECT_EQ(buffer32[2], 0xEF);
    EXPECT_EQ(buffer32[3], 0x01);
    
    // 读取并验证
    uint16_t readUint16 = 0;
    uint32_t readUint32 = 0;
    uint64_t readUint64 = 0;
    
    Endian::ReadBigEndian(buffer16, readUint16);
    EXPECT_EQ(readUint16, uint16Value);
    
    Endian::ReadBigEndian(buffer32, readUint32);
    EXPECT_EQ(readUint32, uint32Value);
    
    Endian::ReadBigEndian(buffer64, readUint64);
    EXPECT_EQ(readUint64, uint64Value);
}

// 测试端序转换在实际应用中的用例
TEST(EndianTest, NetworkByteOrderSimulation) {
    // 网络字节序为大端序，模拟网络通信中的数据传输
    
    // 创建测试值
    const uint16_t portNumber = 8080;
    const uint32_t ipAddress = 0xC0A80102; // 192.168.1.2
    
    // 模拟发送数据
    uint8_t packet[6]; // 保存端口和IP地址
    
    // 写入网络字节序（大端序）
    Endian::WriteBigEndian(packet, portNumber);
    Endian::WriteBigEndian(packet + 2, ipAddress);
    
    // 验证数据格式是否正确
    EXPECT_EQ(packet[0], 0x1F);  // 8080 = 0x1F90
    EXPECT_EQ(packet[1], 0x90);
    EXPECT_EQ(packet[2], 0xC0);  // 192.168.1.2 = 0xC0A80102
    EXPECT_EQ(packet[3], 0xA8);
    EXPECT_EQ(packet[4], 0x01);
    EXPECT_EQ(packet[5], 0x02);
    
    // 模拟接收数据
    uint16_t receivedPort;
    uint32_t receivedIp;
    
    Endian::ReadBigEndian(packet, receivedPort);
    Endian::ReadBigEndian(packet + 2, receivedIp);
    
    // 验证解析结果是否正确
    EXPECT_EQ(receivedPort, portNumber);
    EXPECT_EQ(receivedIp, ipAddress);
} 