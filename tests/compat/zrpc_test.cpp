// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// zrpc 模块单元测试：覆盖 TcpBuffer / NetAddress / ZRpcCodeC / ZRpcController。
// 全部为纯逻辑代码，不依赖网络与 GUI。

#include <gtest/gtest.h>

#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "tcpbuffer.h"
#include "netaddress.h"
#include "rpccontroller.h"
#include "specodec.h"
#include "specdata.h"

using namespace zrpc_ns;

// ============================================================================
// TcpBuffer
// ============================================================================

TEST(TcpBufferTest, InitialState)
{
    TcpBuffer buf(16);
    EXPECT_EQ(buf.getSize(), 16);
    EXPECT_EQ(buf.readAble(), 0);
    EXPECT_EQ(buf.writeAble(), 16);
    EXPECT_EQ(buf.readIndex(), 0);
    EXPECT_EQ(buf.writeIndex(), 0);
}

TEST(TcpBufferTest, WriteWithinCapacityDoesNotGrow)
{
    TcpBuffer buf(16);
    const char data[] = "hello";
    buf.writeToBuffer(data, 5);
    EXPECT_EQ(buf.writeIndex(), 5);
    EXPECT_EQ(buf.readAble(), 5);
    EXPECT_EQ(buf.writeAble(), 11);
    EXPECT_EQ(buf.getSize(), 16);
}

TEST(TcpBufferTest, WriteBeyondCapacityGrowsBuffer)
{
    TcpBuffer buf(4);
    const char data[] = "abcdefhijk";   // 10 bytes
    buf.writeToBuffer(data, 10);
    EXPECT_EQ(buf.readAble(), 10);
    EXPECT_EQ(buf.writeIndex(), 10);
    // 扩容后容量必须 >= 10
    EXPECT_GE(buf.getSize(), 10);
}

TEST(TcpBufferTest, ReadFromEmptyBufferNoop)
{
    TcpBuffer buf(8);
    std::vector<char> out;
    buf.readFromBuffer(out, 4);
    EXPECT_TRUE(out.empty());
    EXPECT_EQ(buf.readIndex(), 0);
}

TEST(TcpBufferTest, ReadPartialThenAdjust)
{
    TcpBuffer buf(32);
    const char data[] = "0123456789ABCDEF";   // 16 bytes
    buf.writeToBuffer(data, 16);

    std::vector<char> out;
    buf.readFromBuffer(out, 6);
    ASSERT_EQ(out.size(), 6u);
    EXPECT_EQ(std::string(out.data(), 6), "012345");
    EXPECT_EQ(buf.readIndex(), 6);
    EXPECT_EQ(buf.readAble(), 10);
}

TEST(TcpBufferTest, ReadMoreThanAvailableClamps)
{
    TcpBuffer buf(32);
    const char data[] = "abc";
    buf.writeToBuffer(data, 3);

    std::vector<char> out;
    buf.readFromBuffer(out, 100);   // 请求超过可读量
    ASSERT_EQ(out.size(), 3u);
    EXPECT_EQ(std::string(out.data(), 3), "abc");
    EXPECT_EQ(buf.readAble(), 0);
}

TEST(TcpBufferTest, ReadTriggersAdjustWhenReadIndexExceedsThird)
{
    // 构造容量足够大的缓冲，多次读使 m_read_index > size/3 触发 adjustBuffer
    TcpBuffer buf(60);
    std::string payload(40, 'X');
    buf.writeToBuffer(payload.data(), 40);
    EXPECT_EQ(buf.getSize(), 60);

    std::vector<char> out;
    buf.readFromBuffer(out, 25);   // read_index=25 > 60/3=20，触发 compact
    ASSERT_EQ(out.size(), 25u);
    // compact 后 read_index 归零
    EXPECT_EQ(buf.readIndex(), 0);
    EXPECT_EQ(buf.readAble(), 15);
    EXPECT_EQ(buf.writeIndex(), 15);
}

TEST(TcpBufferTest, ResizeBufferShrinks)
{
    TcpBuffer buf(32);
    std::string payload(32, 'Y');
    buf.writeToBuffer(payload.data(), 32);

    buf.resizeBuffer(8);   // 缩到 8，保留前 min(8, readAble()) 字节
    EXPECT_EQ(buf.getSize(), 8);
    EXPECT_EQ(buf.readIndex(), 0);
    EXPECT_EQ(buf.readAble(), 8);
}

TEST(TcpBufferTest, RecycleReadNormalAndOverflow)
{
    TcpBuffer buf(16);
    const char data[] = "0123456789";
    buf.writeToBuffer(data, 10);

    buf.recycleRead(3);
    EXPECT_EQ(buf.readIndex(), 3);

    // 越界：j = read_index + index > size，应被忽略
    buf.recycleRead(1000);
    EXPECT_EQ(buf.readIndex(), 3);
}

TEST(TcpBufferTest, RecycleWriteNormalAndOverflow)
{
    TcpBuffer buf(16);
    buf.recycleWrite(4);
    EXPECT_EQ(buf.writeIndex(), 4);

    // 越界：j = write_index + index > size，应被忽略
    buf.recycleWrite(1000);
    EXPECT_EQ(buf.writeIndex(), 4);
}

TEST(TcpBufferTest, GetBufferStringAndVector)
{
    TcpBuffer buf(16);
    const char data[] = "ABCDE";
    buf.writeToBuffer(data, 5);

    std::string s = buf.getBufferString();
    EXPECT_EQ(s, "ABCDE");

    auto v = buf.getBufferVector();
    EXPECT_EQ(v.size(), 16u);
}

TEST(TcpBufferTest, ClearBufferResetsIndices)
{
    TcpBuffer buf(16);
    const char data[] = "xyz";
    buf.writeToBuffer(data, 3);
    buf.clearBuffer();
    EXPECT_EQ(buf.getSize(), 0);
    EXPECT_EQ(buf.readIndex(), 0);
    EXPECT_EQ(buf.writeIndex(), 0);
    EXPECT_EQ(buf.readAble(), 0);
}

// ============================================================================
// NetAddress
// ============================================================================

TEST(NetAddressTest, ValidIpAndSslCredentials)
{
    char key[] = "k";
    char crt[] = "c";
    NetAddress addr("192.168.1.1", 8080, key, crt);
    EXPECT_STREQ(addr.getIP(), "192.168.1.1");
    EXPECT_EQ(addr.getPort(), 8080);
    EXPECT_TRUE(addr.isSSL());
    EXPECT_STREQ(addr.getKey(), "k");
    EXPECT_STREQ(addr.getCrt(), "c");
}

TEST(NetAddressTest, NullIpFallsBackToWildcard)
{
    char key[] = "k";
    char crt[] = "c";
    NetAddress addr(nullptr, 1234, key, crt);
    EXPECT_STREQ(addr.getIP(), "0.0.0.0");
    EXPECT_EQ(addr.getPort(), 1234);
}

TEST(NetAddressTest, EmptyIpFallsBackToWildcard)
{
    char key[] = "k";
    char crt[] = "c";
    NetAddress addr("", 1234, key, crt);
    EXPECT_STREQ(addr.getIP(), "0.0.0.0");
}

TEST(NetAddressTest, MissingKeyDisablesSsl)
{
    char crt[] = "c";
    NetAddress addr("10.0.0.1", 443, nullptr, crt);
    EXPECT_FALSE(addr.isSSL());
}

TEST(NetAddressTest, MissingCrtDisablesSsl)
{
    char key[] = "k";
    NetAddress addr("10.0.0.1", 443, key, nullptr);
    EXPECT_FALSE(addr.isSSL());
}

TEST(NetAddressTest, SslBoolCtor)
{
    NetAddress addr("10.0.0.2", 7000, true);
    EXPECT_STREQ(addr.getIP(), "10.0.0.2");
    EXPECT_EQ(addr.getPort(), 7000);
    EXPECT_TRUE(addr.isSSL());
}

TEST(NetAddressTest, SslBoolCtorNullIp)
{
    NetAddress addr(nullptr, 7000, false);
    EXPECT_STREQ(addr.getIP(), "0.0.0.0");
    EXPECT_FALSE(addr.isSSL());
}

// ============================================================================
// ZRpcController
// ============================================================================

TEST(ZRpcControllerTest, DefaultsAndFailedFlow)
{
    ZRpcController c;
    EXPECT_FALSE(c.Failed());
    EXPECT_EQ(c.ErrorText(), "");
    EXPECT_FALSE(c.IsCanceled());

    c.SetFailed("boom");
    EXPECT_TRUE(c.Failed());
    EXPECT_EQ(c.ErrorText(), "boom");
}

TEST(ZRpcControllerTest, SetErrorCombinesCodeAndInfo)
{
    ZRpcController c;
    c.SetError(42, "nope");
    EXPECT_TRUE(c.Failed());
    EXPECT_EQ(c.ErrorCode(), 42);
    EXPECT_EQ(c.ErrorText(), "nope");
}

TEST(ZRpcControllerTest, MsgSeq)
{
    ZRpcController c;
    EXPECT_EQ(c.MsgSeq(), "");
    c.SetMsgReq("req-1");
    EXPECT_EQ(c.MsgSeq(), "req-1");
}

TEST(ZRpcControllerTest, Timeout)
{
    ZRpcController c;
    EXPECT_EQ(c.Timeout(), 5000);
    c.SetTimeout(100);
    EXPECT_EQ(c.Timeout(), 100);
}

TEST(ZRpcControllerTest, MethodNames)
{
    ZRpcController c;
    c.SetMethodName("foo");
    c.SetMethodFullName("Svc.foo");
    EXPECT_EQ(c.GetMethodName(), "foo");
    EXPECT_EQ(c.GetMethodFullName(), "Svc.foo");
}

TEST(ZRpcControllerTest, PeerAndLocalAddr)
{
    ZRpcController c;
    EXPECT_EQ(c.PeerAddr(), nullptr);
    EXPECT_EQ(c.LocalAddr(), nullptr);

    auto peer = std::make_shared<NetAddress>("1.2.3.4", 1000, false);
    auto local = std::make_shared<NetAddress>("127.0.0.1", 2000, false);
    c.SetPeerAddr(peer);
    c.SetLocalAddr(local);
    EXPECT_STREQ(c.PeerAddr()->getIP(), "1.2.3.4");
    EXPECT_STREQ(c.LocalAddr()->getIP(), "127.0.0.1");
}

TEST(ZRpcControllerTest, NoopOverridesDoNotCrash)
{
    ZRpcController c;
    c.Reset();
    c.StartCancel();
    c.NotifyOnCancel(nullptr);
    SUCCEED();
}

// ============================================================================
// ZRpcCodeC
// ============================================================================

TEST(ZRpcCodeCTest, EncodeNullArgsSafe)
{
    ZRpcCodeC codec;
    SpecDataStruct data;
    codec.encode(nullptr, nullptr);   // 不应崩溃
    EXPECT_FALSE(data.encode_succ);
}

TEST(ZRpcCodeCTest, EncodePbDataEmptyServiceNameReturnsNull)
{
    ZRpcCodeC codec;
    SpecDataStruct data;
    uint32_t len = 0;
    const char *ret = codec.encodePbData(&data, len);
    EXPECT_EQ(ret, nullptr);
    EXPECT_FALSE(data.encode_succ);
}

TEST(ZRpcCodeCTest, EncodePbDataAutoGeneratesMsgReq)
{
    ZRpcCodeC codec;
    SpecDataStruct data;
    data.service_full_name = "Svc.Query";
    data.pb_data = "payload";
    uint32_t len = 0;

    const char *ret = codec.encodePbData(&data, len);
    ASSERT_NE(ret, nullptr);
    EXPECT_GT(len, 0u);
    EXPECT_TRUE(data.encode_succ);
    EXPECT_FALSE(data.msg_req.empty());
    EXPECT_EQ(data.msg_req_len, data.msg_req.size());
    EXPECT_EQ(data.pk_len, len);
    free((void *)ret);
}

TEST(ZRpcCodeCTest, EncodeKeepsProvidedMsgReq)
{
    ZRpcCodeC codec;
    SpecDataStruct data;
    data.service_full_name = "Svc.Query";
    data.msg_req = "fixed-id";
    data.pb_data = "x";
    uint32_t len = 0;
    codec.encodePbData(&data, len);
    EXPECT_EQ(data.msg_req, "fixed-id");
}

TEST(ZRpcCodeCTest, EncodeWritesToBuffer)
{
    ZRpcCodeC codec;
    TcpBuffer buf(64);
    SpecDataStruct data;
    data.service_full_name = "Svc.Query";
    data.pb_data = "hello";
    codec.encode(&buf, &data);
    EXPECT_TRUE(data.encode_succ);
    EXPECT_GT(buf.writeIndex(), 0);
    EXPECT_GT(buf.readAble(), 0);
}

TEST(ZRpcCodeCTest, EncodeDecodeRoundTrip)
{
    // encode→decode 往返：一条用例覆盖大半编解码分支
    ZRpcCodeC codec;
    TcpBuffer buf(128);

    SpecDataStruct out;
    out.service_full_name = "QueryService.query_name";
    out.msg_req = "msg-id-123";
    out.pb_data = "protobuf-bytes-payload";
    out.err_info = "";
    out.err_code = 0;

    codec.encode(&buf, &out);
    ASSERT_TRUE(out.encode_succ) << "encode should succeed";

    SpecDataStruct in;
    codec.decode(&buf, &in);
    ASSERT_TRUE(in.decode_succ) << "decode should succeed";
    EXPECT_EQ(in.msg_req, out.msg_req);
    EXPECT_EQ(in.service_full_name, out.service_full_name);
    EXPECT_EQ(in.pb_data, out.pb_data);
    EXPECT_EQ(in.err_code, 0u);
}

TEST(ZRpcCodeCTest, EncodeDecodeRoundTripWithErrInfo)
{
    ZRpcCodeC codec;
    TcpBuffer buf(256);

    SpecDataStruct out;
    out.service_full_name = "Svc.Do";
    out.msg_req = "id-2";
    out.pb_data = "data2";
    out.err_info = "some error text";
    out.err_code = 7;

    codec.encode(&buf, &out);
    ASSERT_TRUE(out.encode_succ);

    SpecDataStruct in;
    codec.decode(&buf, &in);
    ASSERT_TRUE(in.decode_succ);
    EXPECT_EQ(in.service_full_name, "Svc.Do");
    EXPECT_EQ(in.msg_req, "id-2");
    EXPECT_EQ(in.err_info, "some error text");
    EXPECT_EQ(in.err_code, 7u);
    EXPECT_EQ(in.pb_data, "data2");
}

TEST(ZRpcCodeCTest, DecodeNullArgsSafe)
{
    ZRpcCodeC codec;
    SpecDataStruct data;
    codec.decode(nullptr, nullptr);
    EXPECT_FALSE(data.decode_succ);
}

TEST(ZRpcCodeCTest, DecodeEmptyBufferDoesNotParse)
{
    ZRpcCodeC codec;
    TcpBuffer buf(32);   // 未写入任何数据
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

TEST(ZRpcCodeCTest, DecodePartialPackageDoesNotParse)
{
    // 截断已编码包的尾部（去掉 PB_END），应无法解析完整包
    ZRpcCodeC codec;
    TcpBuffer buf(128);
    SpecDataStruct out;
    out.service_full_name = "Svc.X";
    out.pb_data = "p";
    codec.encode(&buf, &out);

    // 抹掉最后一个字节（PB_END）
    buf.m_buffer[buf.writeIndex() - 1] = 0x00;

    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

// ---------------------------------------------------------------------------
// 以下用例针对 decode 的内部错误返回分支，构造结构合法但内字段非法的原始包。
// 包布局：[PB_START][pk_len:4 BE][msg_req_len:4 BE][msg_req][svc_len:4 BE][svc]
//        [err_code:4 BE][err_info_len:4 BE][err_info][pb_data][checksum:4 BE][PB_END]
// ---------------------------------------------------------------------------

namespace {
constexpr char kPbStart = 0x02;
constexpr char kPbEnd = 0x03;

// 构造一个 TcpBuffer，写入原始字节；writeIndex = bytes.size()
TcpBuffer makeRawBuffer(const std::vector<char> &bytes)
{
    TcpBuffer buf(static_cast<int>(bytes.size()));
    buf.writeToBuffer(bytes.data(), static_cast<int>(bytes.size()));
    return buf;
}

void putU32BE(std::vector<char> &v, uint32_t val)
{
    v.push_back(static_cast<char>((val >> 24) & 0xff));
    v.push_back(static_cast<char>((val >> 16) & 0xff));
    v.push_back(static_cast<char>((val >> 8) & 0xff));
    v.push_back(static_cast<char>(val & 0xff));
}

// 包首尾框架：总长 pkLen 字节 = [START][body: pkLen-2][END]
// body 首 4 字节为 pk_len 字段（= pkLen），其后由 fillInner 填充，不足补 0。
std::vector<char> frameWith(size_t pkLen, std::function<void(std::vector<char> &)> fillInner)
{
    std::vector<char> body;
    putU32BE(body, static_cast<uint32_t>(pkLen));
    fillInner(body);
    if (body.size() < pkLen - 2)
        body.resize(pkLen - 2, 0);
    std::vector<char> out;
    out.push_back(kPbStart);
    out.insert(out.end(), body.begin(), body.end());
    out.push_back(kPbEnd);   // 位于 index = pkLen - 1
    return out;
}
}   // namespace

// msg_req_len_index >= end_index：pk_len 极小（=6），body 容不下 msg_req_len 字段
TEST(ZRpcCodeCTest, DecodeFrameTooSmallForMsgReqLen)
{
    auto raw = frameWith(6, [](std::vector<char> &) {});
    TcpBuffer buf = makeRawBuffer(raw);
    ZRpcCodeC codec;
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

// msg_req_len == 0
TEST(ZRpcCodeCTest, DecodeZeroMsgReqLenRejected)
{
    auto raw = frameWith(10, [](std::vector<char> &body) {
        // body[4..7] = msg_req_len = 0
        putU32BE(body, 0);
    });
    TcpBuffer buf = makeRawBuffer(raw);
    ZRpcCodeC codec;
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

// service_name_len_index >= end_index：包刚好结束在 msg_req 之后
TEST(ZRpcCodeCTest, DecodeTruncatedAtServiceNameLen)
{
    auto raw = frameWith(11, [](std::vector<char> &body) {
        putU32BE(body, 1);          // msg_req_len = 1
        body.push_back('X');        // msg_req
    });
    TcpBuffer buf = makeRawBuffer(raw);
    ZRpcCodeC codec;
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

// service_name_index >= end_index：service_name_len 字段存在但 service_name 越界
TEST(ZRpcCodeCTest, DecodeTruncatedAtServiceName)
{
    auto raw = frameWith(15, [](std::vector<char> &body) {
        putU32BE(body, 1);          // msg_req_len = 1
        body.push_back('X');        // msg_req
        putU32BE(body, 99);         // service_name_len = 99
    });
    TcpBuffer buf = makeRawBuffer(raw);
    ZRpcCodeC codec;
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

// service_name_len > pk_len
TEST(ZRpcCodeCTest, DecodeServiceNameLenExceedsPkLen)
{
    auto raw = frameWith(16, [](std::vector<char> &body) {
        putU32BE(body, 1);          // msg_req_len = 1
        body.push_back('X');        // msg_req
        putU32BE(body, 9999);       // service_name_len = 9999 > pk_len=16
        body.push_back(0);          // 填充
    });
    TcpBuffer buf = makeRawBuffer(raw);
    ZRpcCodeC codec;
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}

// err_info_len_index >= end_index：包在 err_code 之后截断
TEST(ZRpcCodeCTest, DecodeTruncatedAtErrInfoLen)
{
    auto raw = frameWith(20, [](std::vector<char> &body) {
        putU32BE(body, 1);          // msg_req_len = 1
        body.push_back('X');        // msg_req
        putU32BE(body, 1);          // service_name_len = 1
        body.push_back('S');        // service_name
        putU32BE(body, 0);          // err_code = 0
    });
    TcpBuffer buf = makeRawBuffer(raw);
    ZRpcCodeC codec;
    SpecDataStruct in;
    codec.decode(&buf, &in);
    EXPECT_FALSE(in.decode_succ);
}
