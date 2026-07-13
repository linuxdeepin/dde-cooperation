// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Coverage for Timer non-network methods.

#include <catch2/catch_all.hpp>
#include <asio/service.h>
#include <asio/timer.h>

#include <thread>
#include <chrono>

using namespace NetUtil::Asio;

TEST_CASE("Timer construct", "[timer][ctor]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    SECTION("default construct") {
        Timer t(service);
    }

    SECTION("construct with action") {
        Timer t(service, [](bool) {});
    }

    SECTION("construct with action and UtcTime") {
        Timer t(service, [](bool) {}, BaseKit::UtcTime());
    }

    SECTION("construct with action and Timespan") {
        Timer t(service, [](bool) {}, BaseKit::Timespan::seconds(1));
    }

    SECTION("construct with UtcTime") {
        Timer t(service, BaseKit::UtcTime());
    }

    SECTION("construct with Timespan") {
        Timer t(service, BaseKit::Timespan::seconds(1));
    }

    service->Stop();
}

TEST_CASE("Timer setup and wait", "[timer][ops]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    bool called = false;

    SECTION("setup with action+timespan then wait") {
        REQUIRE(t.Setup([&called](bool canceled) {
            if (!canceled) called = true;
        }, BaseKit::Timespan::milliseconds(50)));
        try { t.WaitAsync(); } catch (...) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        REQUIRE(called);
    }

    service->Stop();
}

TEST_CASE("Timer setup and cancel", "[timer][cancel]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    bool called = false;

    REQUIRE(t.Setup([&called](bool canceled) {
        if (!canceled) called = true;
    }, BaseKit::Timespan::seconds(10)));
    try { t.WaitAsync(); } catch (...) {}
    REQUIRE(t.Cancel());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE_FALSE(called);

    service->Stop();
}

TEST_CASE("Timer setup with timespan only", "[timer][setup]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    REQUIRE(t.Setup(BaseKit::Timespan::milliseconds(10)));
    REQUIRE(t.WaitSync()); // synchronous wait
    service->Stop();
}

TEST_CASE("Timer setup with action only", "[timer][action]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    REQUIRE(t.Setup([](bool) {}));
    service->Stop();
}

TEST_CASE("Timer setup with UtcTime", "[timer][utctime]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    Timer t(service);
    auto future = BaseKit::UtcTime() + BaseKit::Timespan::milliseconds(10);
    REQUIRE(t.Setup(future));
    REQUIRE(t.WaitSync());
    service->Stop();
}

TEST_CASE("Timer multiple setup", "[timer][repeat]")
{
    auto service = std::make_shared<Service>();
    service->Start();

    int count = 0;
    Timer t(service);

    REQUIRE(t.Setup([&count](bool canceled) {
        if (!canceled) count++;
    }, BaseKit::Timespan::milliseconds(20)));
    try { t.WaitAsync(); } catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(t.Setup([&count](bool canceled) {
        if (!canceled) count++;
    }, BaseKit::Timespan::milliseconds(20)));
    try { t.WaitAsync(); } catch (...) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    service->Stop();
}
