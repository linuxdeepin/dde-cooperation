// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// CommandParser 单元测试 —— src/singleton/commandparser.cpp (0% → 80%+)
// 注意: process(arguments) 内部调用 QCommandLineParser::process(),
// 该方法遇到 --help/--version/未知选项会调用 std::exit(), 故只传合法选项。

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QStringList>

#include "singleton/commandparser.h"

namespace {
// 复用 coop_tests main.cpp 里已经创建的 QApplication (offscreen).
// 测试之间需要重置 parser 状态: 重新构造 instance 不可行 (单例),
// 所以每个用例传入独立的参数列表触发 cmdParser->process() 重置状态.
}

TEST(CommandParserTest, InstanceReturnsSameReference)
{
    auto &a = CommandParser::instance();
    auto &b = CommandParser::instance();
    EXPECT_EQ(&a, &b);
}

TEST(CommandParserTest, ProcessValidSendFilesOptionReturnsPositionalArgs)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-s"),
                    QStringLiteral("file1.txt"), QStringLiteral("file2.txt")});
    const QStringList args = parser.processCommand(QStringLiteral("s"));
    EXPECT_EQ(args.size(), 2);
    EXPECT_EQ(args.at(0).toStdString(), "file1.txt");
    EXPECT_EQ(args.at(1).toStdString(), "file2.txt");
}

TEST(CommandParserTest, ProcessValidLongSendFilesOption)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("--send-files"),
                    QStringLiteral("a.bin")});
    const QStringList args = parser.processCommand(QStringLiteral("send-files"));
    EXPECT_EQ(args.size(), 1);
}

TEST(CommandParserTest, ProcessDetailOptionIsRecognized)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-d")});
    const QStringList args = parser.processCommand(QStringLiteral("d"));
    // -d 不带位置参数, isSet=true 但 positional 为空
    EXPECT_TRUE(args.isEmpty());
}

TEST(CommandParserTest, ProcessMinimizeOptionIsRecognized)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-m")});
    // 调用 processCommand 验证选项被注册且不报错
    const QStringList args = parser.processCommand(QStringLiteral("m"));
    EXPECT_TRUE(args.isEmpty());
}

TEST(CommandParserTest, ProcessForwardOptionIsRecognized)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-f"),
                    QStringLiteral("192.168.1.10"), QStringLiteral("device1"),
                    QStringLiteral("photo.jpg")});
    const QStringList args = parser.processCommand(QStringLiteral("f"));
    EXPECT_EQ(args.size(), 3);
}

TEST(CommandParserTest, ProcessCommandReturnsEmptyWhenOptionNotSet)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-d")});
    const QStringList args = parser.processCommand(QStringLiteral("s"));
    EXPECT_TRUE(args.isEmpty());
}

TEST(CommandParserTest, ProcessCommandUnknownShortNameReturnsEmpty)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-d")});
    // 未知短名: isSet 返回 false
    const QStringList args = parser.processCommand(QStringLiteral("zzz"));
    EXPECT_TRUE(args.isEmpty());
}

TEST(CommandParserTest, ProcessEmptyArgumentsDoesNotThrow)
{
    auto &parser = CommandParser::instance();
    // 空参数列表: process 不会抛出, 也不会 exit
    EXPECT_NO_THROW(parser.process({QStringLiteral("app")}));
}

TEST(CommandParserTest, ProcessWithMultipleOptionsAndPositionalArgs)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("-d"),
                    QStringLiteral("-m"), QStringLiteral("-s"),
                    QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")});
    const QStringList sendArgs = parser.processCommand(QStringLiteral("s"));
    EXPECT_EQ(sendArgs.size(), 3);
    // detail/minimize 选项虽然 isSet=true 但 positionalArguments 是全局的
    const QStringList detailArgs = parser.processCommand(QStringLiteral("d"));
    EXPECT_EQ(detailArgs.size(), 3);
}

TEST(CommandParserTest, ProcessNoArgOverloadDoesNotThrow)
{
    auto &parser = CommandParser::instance();
    // process() 用 qApp->arguments() —— 在测试环境不会含 --help
    EXPECT_NO_THROW(parser.process());
}

TEST(CommandParserTest, ProcessCommandLongNameDetail)
{
    auto &parser = CommandParser::instance();
    parser.process({QStringLiteral("app"), QStringLiteral("--detail")});
    // 长名 detail 也应被识别
    const QStringList argsShort = parser.processCommand(QStringLiteral("d"));
    const QStringList argsLong = parser.processCommand(QStringLiteral("detail"));
    EXPECT_TRUE(argsShort.isEmpty());
    EXPECT_TRUE(argsLong.isEmpty());
}
