// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    // offscreen 由 test-prj-running.sh 设置; 此处兜底防止本地直接跑
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
