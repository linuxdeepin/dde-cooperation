// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// Custom main for coop_tests.
//
// Why a custom main (instead of linking GTest::Main):
//  - Several cooperation-core singletons (ShareHelper::instance() in particular)
//    lazily create QWidget subclasses (CooperationTaskDialog), which requires a
//    QApplication — not just QCoreApplication. gtest_main does not create one.
//  - A QCoreApplication used to be installed via a global testing Environment,
//    but QApplication is a QWidget-capable superset and works for every test.
//  - At process exit a static ConfigManager destructor races against Qt
//    teardown and calls free() on an already-freed pointer, aborting with
//    SIGABRT (exit 134). The abort skips __gcov_dump()'s atexit handler, so
//    no .gcda coverage files are written and lcov reports zero coverage for
//    coop_tests. A custom main that explicitly calls __gcov_dump()/__gcov_flush()
//    before returning — and returns normally instead of letting the abort
//    happen — produces .gcda files.
//
// __gcov_dump() (gcc 11+) flushes all coverage counters; on older toolchains
// __gcov_flush() is the equivalent. We declare them as extern "C" weak so the
// binary still links if the toolchain lacks one of them.

#include <QApplication>
#include <gtest/gtest.h>

extern "C" void __gcov_dump() __attribute__((weak));
extern "C" void __gcov_flush() __attribute__((weak));

static void flushCoverage()
{
    if (__gcov_dump) {
        __gcov_dump();
    } else if (__gcov_flush) {
        __gcov_flush();
    }
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    // Flush coverage counters *before* static destructors run (the
    // ConfigManager/QApplication teardown races cause the exit-134 abort that
    // would otherwise skip gcov finalization).
    flushCoverage();
    return result;
}
