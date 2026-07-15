// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "singleapplication.h"

using namespace deepin_cross;

// SingleApplication destructor calls _exit(0), so RUN_ALL_TESTS result
// is never returned. Coverage is flushed via __gcov_dump() before _exit.
int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    SingleApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
    // app destructor fires _exit(0) at process teardown
    // — __gcov_dump() is called first under ENABLE_AUTO_UNIT_TEST.
    return 0; // never reached
}
