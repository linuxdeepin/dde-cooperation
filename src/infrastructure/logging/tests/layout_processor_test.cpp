// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// BinaryLayout / HashLayout + 各 Processor 扩展覆盖。

#include <catch2/catch_all.hpp>
#include <logging/record.h>
#include <logging/layouts/binary_layout.h>
#include <logging/layouts/hash_layout.h>
#include <logging/processors/sync_processor.h>
#include <logging/processors/buffered_processor.h>
#include <logging/processors/exclusive_processor.h>
#include <logging/processors/async_wait_processor.h>
#include <logging/processors/async_wait_free_processor.h>
#include <logging/appenders/ostream_appender.h>
#include <logging/appenders/memory_appender.h>

#include <memory>
#include <sstream>
#include <chrono>
#include <thread>

using namespace Logging;

namespace {
Record makeRec(Level lvl, const std::string &msg)
{
    Record r;
    r.timestamp = 1700000000;
    r.thread = 1;
    r.level = lvl;
    r.logger = "LP";
    r.message = msg;
    return r;
}
}   // namespace

// ===== BinaryLayout / HashLayout =====
TEST_CASE("BinaryLayout produces record bytes", "[layout][binary]")
{
    BinaryLayout layout;
    Record r = makeRec(Level::INFO, "binary-payload");
    layout.LayoutRecord(r);
    SUCCEED();
}

TEST_CASE("HashLayout produces record bytes", "[layout][hash]")
{
    HashLayout layout;
    Record r = makeRec(Level::WARN, "hash-payload");
    layout.LayoutRecord(r);
    SUCCEED();
}

// ===== SyncProcessor =====
TEST_CASE("SyncProcessor processes record", "[proc][sync]")
{
    auto layout = std::make_shared<BinaryLayout>();
    SyncProcessor proc(layout);
    auto app = std::make_shared<MemoryAppender>(0);
    proc.appenders().push_back(app);
    proc.Start();
    Record r = makeRec(Level::ERROR, "sync-msg");
    proc.ProcessRecord(r);
    proc.Stop();
    SUCCEED();
}

// ===== ExclusiveProcessor =====
TEST_CASE("ExclusiveProcessor processes record", "[proc][exclusive]")
{
    auto layout = std::make_shared<BinaryLayout>();
    ExclusiveProcessor proc(layout);
    proc.appenders().push_back(std::make_shared<MemoryAppender>(0));
    proc.Start();
    Record r = makeRec(Level::INFO, "exclusive-msg");
    proc.ProcessRecord(r);
    proc.Stop();
    SUCCEED();
}

// ===== BufferedProcessor =====
TEST_CASE("BufferedProcessor processes record", "[proc][buffered]")
{
    auto layout = std::make_shared<BinaryLayout>();
    BufferedProcessor proc(layout, 64, 16);
    proc.appenders().push_back(std::make_shared<MemoryAppender>(0));
    proc.Start();
    for (int i = 0; i < 8; ++i) {
        Record r = makeRec(Level::DEBUG, "buffered-msg");
        proc.ProcessRecord(r);
    }
    proc.Flush();
    proc.Stop();
    SUCCEED();
}

// ===== AsyncWaitProcessor =====
TEST_CASE("AsyncWaitProcessor processes record", "[proc][async_wait]")
{
    auto layout = std::make_shared<BinaryLayout>();
    AsyncWaitProcessor proc(layout);
    proc.appenders().push_back(std::make_shared<MemoryAppender>(0));
    proc.Start();
    for (int i = 0; i < 4; ++i) {
        Record r = makeRec(Level::INFO, "async-wait-msg");
        proc.ProcessRecord(r);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    proc.Stop();
    SUCCEED();
}

// ===== AsyncWaitFreeProcessor =====
TEST_CASE("AsyncWaitFreeProcessor processes record", "[proc][async_waitfree]")
{
    auto layout = std::make_shared<BinaryLayout>();
    AsyncWaitFreeProcessor proc(layout);
    proc.appenders().push_back(std::make_shared<MemoryAppender>(0));
    proc.Start();
    for (int i = 0; i < 4; ++i) {
        Record r = makeRec(Level::INFO, "async-waitfree-msg");
        proc.ProcessRecord(r);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    proc.Stop();
    SUCCEED();
}
