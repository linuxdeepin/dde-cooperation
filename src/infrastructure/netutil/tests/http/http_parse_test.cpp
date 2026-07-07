// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// HTTPRequest / HTTPResponse 编解码与各 Make*Request/Response 工厂覆盖。

#include <catch2/catch_all.hpp>
#include <http/http_request.h>
#include <http/http_response.h>

#include <cstring>
#include <string>

using namespace NetUtil::HTTP;

// ===== Make*Request 系列 =====
TEST_CASE("HTTPRequest MakeGet", "[http_request][make]")
{
    HTTPRequest r;
    r.MakeGetRequest("/api/items");
    REQUIRE(r.method() == "GET");
    REQUIRE(r.url() == "/api/items");
    REQUIRE(r.protocol() == "HTTP/1.1");
}

TEST_CASE("HTTPRequest MakePost", "[http_request][make]")
{
    HTTPRequest r;
    r.MakePostRequest("/api/items", "body-data", "application/json");
    REQUIRE(r.method() == "POST");
    REQUIRE(r.url() == "/api/items");
}

TEST_CASE("HTTPRequest MakePut", "[http_request][make]")
{
    HTTPRequest r;
    r.MakePutRequest("/api/items/1", "put-body");
    REQUIRE(r.method() == "PUT");
}

TEST_CASE("HTTPRequest MakeDelete", "[http_request][make]")
{
    HTTPRequest r;
    r.MakeDeleteRequest("/api/items/1");
    REQUIRE(r.method() == "DELETE");
}

TEST_CASE("HTTPRequest MakeHead", "[http_request][make]")
{
    HTTPRequest r;
    r.MakeHeadRequest("/api/items");
    REQUIRE(r.method() == "HEAD");
}

TEST_CASE("HTTPRequest MakeOptions", "[http_request][make]")
{
    HTTPRequest r;
    r.MakeOptionsRequest("*");
    REQUIRE(r.method() == "OPTIONS");
}

TEST_CASE("HTTPRequest MakeTrace", "[http_request][make]")
{
    HTTPRequest r;
    r.MakeTraceRequest("/api/items");
    REQUIRE(r.method() == "TRACE");
}

// ===== 序列化与 cache/string =====
TEST_CASE("HTTPRequest string serialization", "[http_request][ser]")
{
    HTTPRequest r;
    r.MakeGetRequest("/x");
    r.SetHeader("Host", "example.com");
    std::string s = r.string();
    REQUIRE_FALSE(s.empty());
    // 序列化结果至少含方法名
    REQUIRE(s.find("GET") != std::string::npos);
}

TEST_CASE("HTTPRequest Clear resets", "[http_request][clear]")
{
    HTTPRequest r;
    r.MakeGetRequest("/x");
    r.Clear();
    REQUIRE(r.empty());
}

// ===== Header / Cookie 操作 =====
TEST_CASE("HTTPRequest SetHeader and query", "[http_request][hdr]")
{
    HTTPRequest r;
    r.MakeGetRequest("/x");
    r.SetHeader("X-Test", "1");
    REQUIRE(r.headers() >= 1);
}

// ===== ReceiveHeader / ReceiveBody 解析 =====
// ===== HTTPResponse Make* 系列 =====
TEST_CASE("HTTPResponse MakeOK", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeOKResponse(200);
    REQUIRE(r.status() == 200);
}

TEST_CASE("HTTPResponse MakeGet", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeGetResponse("hello", "text/plain");
    REQUIRE(r.status() == 200);
    REQUIRE(r.body() == "hello");
}

TEST_CASE("HTTPResponse MakeError default", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeErrorResponse("oops");
    REQUIRE(r.status() == 500);
}

TEST_CASE("HTTPResponse MakeError custom status", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeErrorResponse(404, "not found", "text/plain");
    REQUIRE(r.status() == 404);
}

TEST_CASE("HTTPResponse MakeHead", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeHeadResponse();
    REQUIRE(r.status() == 200);
}

TEST_CASE("HTTPResponse MakeOptions", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeOptionsResponse();
    REQUIRE(r.status() == 200);
}

TEST_CASE("HTTPResponse MakeTrace", "[http_response][make]")
{
    HTTPResponse r;
    r.MakeTraceResponse("TRACE /x HTTP/1.1\r\n\r\n");
    REQUIRE(r.status() == 200);
}

// ===== Response 解析 =====
TEST_CASE("HTTPResponse Clear", "[http_response][clear]")
{
    HTTPResponse r;
    r.MakeOKResponse();
    r.Clear();
    REQUIRE(r.empty());
}

TEST_CASE("HTTPResponse string serialization", "[http_response][ser]")
{
    HTTPResponse r;
    r.MakeGetResponse("abc", "text/plain");
    std::string s = r.string();
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.find("HTTP/1.1") != std::string::npos);
}

// ===== 工厂往返：客户端发请求，服务端解析 =====
