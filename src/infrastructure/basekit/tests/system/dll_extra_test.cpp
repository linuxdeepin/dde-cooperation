// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include "system/dll.h"
#include "system/exceptions.h"

#include <string>

using namespace BaseKit;

namespace {
const char* kLibz = "/usr/lib/x86_64-linux-gnu/libz.so";
bool libzAvailable()
{
    return Path(kLibz).IsExists();
}
}

TEST(DLLExtraTest, PrefixIsLibOnLinux)
{
    EXPECT_EQ(DLL::prefix(), "lib");
}

TEST(DLLExtraTest, ExtensionIsSoOnLinux)
{
    EXPECT_EQ(DLL::extension(), ".so");
}

TEST(DLLExtraTest, DefaultConstructorIsNotLoaded)
{
    DLL dll;
    EXPECT_FALSE((bool)dll);
    EXPECT_FALSE(dll.IsLoaded());
}

TEST(DLLExtraTest, ConstructWithPathNoLoadAssignsOnly)
{
    if (!libzAvailable()) GTEST_SKIP();
    DLL dll(Path(kLibz), false);
    EXPECT_FALSE(dll.IsLoaded());
    EXPECT_EQ(dll.path().string(), kLibz);
}

TEST(DLLExtraTest, LoadWithPathTwoArgForm)
{
    if (!libzAvailable()) GTEST_SKIP();
    DLL dll;
    EXPECT_TRUE(dll.Load(Path(kLibz)));
    EXPECT_TRUE(dll.IsLoaded());
    EXPECT_TRUE((bool)dll);
    dll.Unload();
    EXPECT_FALSE(dll.IsLoaded());
}

TEST(DLLExtraTest, LoadFailsOnInvalidPathReturnsFalse)
{
    DLL dll;
    EXPECT_FALSE(dll.Load(Path("/no/such/lib_xyzzy.so")));
    EXPECT_FALSE(dll.IsLoaded());
}

TEST(DLLExtraTest, CopyConstructorKeepsPathNotLoaded)
{
    if (!libzAvailable()) GTEST_SKIP();
    DLL original(Path(kLibz), true);
    ASSERT_TRUE(original.IsLoaded());
    DLL copied(original);
    EXPECT_FALSE(copied.IsLoaded());
    EXPECT_EQ(copied.path().string(), kLibz);
}

TEST(DLLExtraTest, CopyAssignmentFromDll)
{
    if (!libzAvailable()) GTEST_SKIP();
    DLL original(Path(kLibz), true);
    DLL target;
    target = original;
    EXPECT_FALSE(target.IsLoaded());
    EXPECT_EQ(target.path().string(), kLibz);
}

TEST(DLLExtraTest, AssignmentFromPath)
{
    if (!libzAvailable()) GTEST_SKIP();
    DLL dll;
    dll = Path(kLibz);
    EXPECT_FALSE(dll.IsLoaded());
    EXPECT_EQ(dll.path().string(), kLibz);
}

TEST(DLLExtraTest, IsResolveLoadedSymbol)
{
    if (!libzAvailable()) GTEST_SKIP();
    DLL dll(Path(kLibz), true);
    ASSERT_TRUE(dll.IsLoaded());
    EXPECT_TRUE(dll.IsResolve("zlibVersion"));
    EXPECT_FALSE(dll.IsResolve("missing_symbol_zzz"));
}

TEST(DLLExtraTest, RelativePathGetsPrefixAndExtension)
{
    DLL dll(Path("foo"), false);
    std::string p = dll.path().string();
    EXPECT_NE(p.find("libfoo"), std::string::npos);
    EXPECT_NE(p.find(".so"), std::string::npos);
}
