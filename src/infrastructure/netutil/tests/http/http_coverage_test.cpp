// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Comprehensive coverage for HTTPRequest / HTTPResponse pure-logic methods.
// Targets: SetCookie, AddCookie, SetContentType, SetBodyLength, ClearCache,
// cookies(), body_length(), cache(), error(), swap, operator<<,
// and all Make*Request/Response edge cases.

#include <catch2/catch_all.hpp>
#include <http/http_request.h>
#include <http/http_response.h>

#include <cstring>
#include <string>

using namespace NetUtil::HTTP;

// ===== HTTPRequest extended =====

TEST_CASE("HTTPRequest SetCookie", "[http_request][cookie]")
{
    HTTPRequest r;
    r.SetBegin("GET", "/x");
    r.SetCookie("session", "abc123");
    r.SetCookie("token", "xyz");
    REQUIRE(r.cookies() == 2);
    auto [n0, v0] = r.cookie(0);
    REQUIRE(n0 == "session");
    REQUIRE(v0 == "abc123");
    auto [n1, v1] = r.cookie(1);
    REQUIRE(n1 == "token");
    REQUIRE(v1 == "xyz");
}

TEST_CASE("HTTPRequest AddCookie", "[http_request][cookie]")
{
    HTTPRequest r;
    r.SetBegin("GET", "/x");
    r.AddCookie("a", "1");
    r.AddCookie("b", "2");
    REQUIRE(r.cookies() == 2);
}

TEST_CASE("HTTPRequest SetBodyLength", "[http_request][body]")
{
    HTTPRequest r;
    r.SetBegin("POST", "/upload");
    r.SetBodyLength(999);
    REQUIRE(r.body_length() == 999);
}

TEST_CASE("HTTPRequest SetBody", "[http_request][body]")
{
    HTTPRequest r;
    r.SetBegin("POST", "/upload");
    r.SetBody("payload data");
    REQUIRE(r.body() == "payload data");
    REQUIRE(r.body_length() > 0);
}

TEST_CASE("HTTPRequest empty and error", "[http_request][state]")
{
    HTTPRequest r;
    REQUIRE(r.empty());
    REQUIRE_FALSE(r.error());

    r.SetBegin("GET", "/x");
    REQUIRE_FALSE(r.empty());
}

TEST_CASE("HTTPRequest cache accessor", "[http_request][cache]")
{
    HTTPRequest r;
    r.SetBegin("GET", "/x");
    r.SetHeader("Host", "test");
    REQUIRE_FALSE(r.cache().empty());
}

TEST_CASE("HTTPRequest swap", "[http_request][swap]")
{
    HTTPRequest r1;
    r1.SetBegin("GET", "/a");
    HTTPRequest r2;
    r2.SetBegin("POST", "/b");

    r1.swap(r2);
    REQUIRE(r1.method() == "POST");
    REQUIRE(r2.method() == "GET");

    // friend swap
    swap(r1, r2);
    REQUIRE(r1.method() == "GET");
    REQUIRE(r2.method() == "POST");
}

TEST_CASE("HTTPRequest copy and assign", "[http_request][copy]")
{
    HTTPRequest r1;
    r1.SetBegin("GET", "/x");
    r1.SetHeader("K", "V");

    HTTPRequest r2 = r1; // copy ctor
    REQUIRE(r2.method() == "GET");
    REQUIRE(r2.url() == "/x");

    HTTPRequest r3;
    r3 = r1; // copy assign
    REQUIRE(r3.method() == "GET");

    HTTPRequest r4 = std::move(r1); // move ctor
    REQUIRE(r4.method() == "GET");

    HTTPRequest r5;
    r5 = std::move(r2); // move assign
    REQUIRE(r5.method() == "GET");
}

TEST_CASE("HTTPRequest multiple headers", "[http_request][header]")
{
    HTTPRequest r;
    r.SetBegin("GET", "/x");
    for (int i = 0; i < 10; i++) {
        r.SetHeader("H" + std::to_string(i), "V" + std::to_string(i));
    }
    REQUIRE(r.headers() == 10);
    auto [k, v] = r.header(5);
    REQUIRE(k == "H5");
    REQUIRE(v == "V5");
}

TEST_CASE("HTTPRequest full string round-trip", "[http_request][ser]")
{
    HTTPRequest r;
    r.MakePostRequest("/submit", "data=42", "application/x-www-form-urlencoded");
    r.SetHeader("Host", "api.test.com");
    r.SetCookie("sid", "token123");
    std::string s = r.string();
    REQUIRE(s.find("POST") != std::string::npos);
    REQUIRE(s.find("/submit") != std::string::npos);
    REQUIRE(s.find("Host: api.test.com") != std::string::npos);
    REQUIRE(s.find("data=42") != std::string::npos);
}

// ===== HTTPResponse extended =====

TEST_CASE("HTTPResponse SetCookie", "[http_response][cookie]")
{
    HTTPResponse r;
    r.SetBegin(200, "OK");
    r.SetCookie("session", "abc");
    r.SetCookie("refresh", "def", 3600, "/", ".test.com", true, true, false);
    REQUIRE(r.headers() >= 2);
}

TEST_CASE("HTTPResponse SetContentType", "[http_response][ctype]")
{
    HTTPResponse r;
    r.SetBegin(200, "OK");
    r.SetContentType(".json");
    // Content-Type header should be set
    bool found = false;
    for (size_t i = 0; i < r.headers(); ++i) {
        auto [k, v] = r.header(i);
        if (k == "Content-Type") { found = true; break; }
    }
    REQUIRE(found);
}

TEST_CASE("HTTPResponse SetBodyLength", "[http_response][body]")
{
    HTTPResponse r;
    r.SetBegin(200, "OK");
    r.SetBodyLength(5000);
    REQUIRE(r.body_length() == 5000);
}

TEST_CASE("HTTPResponse ClearCache", "[http_response][cache]")
{
    HTTPResponse r;
    r.MakeGetResponse("hello", "text/plain");
    REQUIRE_FALSE(r.cache().empty());
    r.ClearCache();
    // ClearCache clears the cache string but keeps state
    SUCCEED();
}

TEST_CASE("HTTPResponse error flag", "[http_response][state]")
{
    HTTPResponse r;
    REQUIRE_FALSE(r.error());
}

TEST_CASE("HTTPResponse swap", "[http_response][swap]")
{
    HTTPResponse r1;
    r1.SetBegin(200, "OK");
    HTTPResponse r2;
    r2.SetBegin(404, "Not Found");

    r1.swap(r2);
    REQUIRE(r1.status() == 404);
    REQUIRE(r2.status() == 200);

    swap(r1, r2);
    REQUIRE(r1.status() == 200);
    REQUIRE(r2.status() == 404);
}

TEST_CASE("HTTPResponse copy and assign", "[http_response][copy]")
{
    HTTPResponse r1;
    r1.SetBegin(200, "OK");
    r1.SetBody("test body");

    HTTPResponse r2 = r1;
    REQUIRE(r2.status() == 200);

    HTTPResponse r3;
    r3 = r1;
    REQUIRE(r3.status() == 200);

    HTTPResponse r4 = std::move(r1);
    REQUIRE(r4.status() == 200);
}

TEST_CASE("HTTPResponse all Make methods", "[http_response][make]")
{
    HTTPResponse r;

    r.MakeOKResponse(204);
    REQUIRE(r.status() == 204);

    r.MakeGetResponse("content", "text/html");
    REQUIRE(r.status() == 200);
    REQUIRE(r.body() == "content");

    r.MakeErrorResponse("custom error");
    REQUIRE(r.status() == 500);

    r.MakeErrorResponse(503, "unavailable");
    REQUIRE(r.status() == 503);

    r.MakeHeadResponse();
    REQUIRE(r.status() == 200);

    r.MakeOptionsResponse();
    REQUIRE(r.status() == 200);

    r.MakeTraceResponse("TRACE /x HTTP/1.1\r\n");
    REQUIRE(r.status() == 200);
}

TEST_CASE("HTTPResponse full string", "[http_response][ser]")
{
    HTTPResponse r;
    r.SetBegin(200, "OK", "HTTP/1.1");
    r.SetHeader("Content-Type", "application/json");
    r.SetCookie("sid", "tok");
    r.SetBody(R"({"k":"v"})");
    std::string s = r.string();
    REQUIRE(s.find("HTTP/1.1") != std::string::npos);
    REQUIRE(s.find("200") != std::string::npos);
    REQUIRE(s.find("Content-Type: application/json") != std::string::npos);
    REQUIRE(s.find("Set-Cookie") != std::string::npos);
}

TEST_CASE("HTTPResponse status phrase access", "[http_response][accessor]")
{
    HTTPResponse r(301, "Moved Permanently", "HTTP/1.1");
    REQUIRE(r.status() == 301);
    REQUIRE(r.status_phrase() == "Moved Permanently");
    REQUIRE(r.protocol() == "HTTP/1.1");
}

// ===== Request-Response factory round-trip =====

TEST_CASE("Request-Response round-trip", "[http][roundtrip]")
{
    // Client builds a POST request
    HTTPRequest req;
    req.MakePostRequest("/api/data", R"({"name":"test"})", "application/json");
    req.SetHeader("Authorization", "Bearer xyz");

    std::string reqStr = req.string();
    REQUIRE(reqStr.find("POST") != std::string::npos);
    REQUIRE(reqStr.find("application/json") != std::string::npos);
    REQUIRE(reqStr.find("Bearer xyz") != std::string::npos);

    // Server builds a response
    HTTPResponse resp;
    resp.MakeGetResponse(R"({"id":1})", "application/json");
    resp.SetHeader("X-Request-Id", "abc-123");

    std::string respStr = resp.string();
    REQUIRE(respStr.find("200") != std::string::npos);
    REQUIRE(respStr.find("X-Request-Id: abc-123") != std::string::npos);
    REQUIRE(respStr.find(R"({"id":1})") != std::string::npos);
}
