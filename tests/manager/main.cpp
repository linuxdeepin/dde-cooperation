// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
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
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    flushCoverage();
    return result;
}
