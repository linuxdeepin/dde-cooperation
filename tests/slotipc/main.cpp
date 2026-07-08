// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// slotipc 测试自定义 main (参考 tests/dt/main.cpp)
// slotipc 无 GUI 依赖, QCoreApplication 即可; __gcov_dump 在静态析构前 flush。

#include <QCoreApplication>
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
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    flushCoverage();
    return result;
}
