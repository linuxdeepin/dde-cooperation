// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "webbinder.h"

// WebBinder is a Singleton; clear before/after each test for determinism.
class WebBinderTest : public ::testing::Test {
protected:
    void SetUp() override { WebBinder::GetInstance().clear(); }
    void TearDown() override { WebBinder::GetInstance().clear(); }
};

TEST_F(WebBinderTest, BindUnbindBasic)
{
    EXPECT_EQ(WebBinder::GetInstance().bind("/files", "/home/user/docs"), 0);
    EXPECT_EQ(WebBinder::GetInstance().unbind("/files"), 0);
}

TEST_F(WebBinderTest, BindDuplicateReturnsError)
{
    EXPECT_EQ(WebBinder::GetInstance().bind("/files", "/home/user/docs"), 0);
    EXPECT_EQ(WebBinder::GetInstance().bind("/files", "/home/user/other"), -1);
}

TEST_F(WebBinderTest, UnbindNonExistentReturnsError)
{
    EXPECT_EQ(WebBinder::GetInstance().unbind("/nofile"), -1);
}

TEST_F(WebBinderTest, BindMismatchedTypeReturnsError)
{
    // webDir ends with '/' (isFile matches) but diskDir does not
    EXPECT_EQ(WebBinder::GetInstance().bind("/folder/", "/home/user/docs"), -4);
    // vice versa
    EXPECT_EQ(WebBinder::GetInstance().bind("/folder", "/home/user/docs/"), -4);
}

TEST_F(WebBinderTest, BindBothFolders)
{
    EXPECT_EQ(WebBinder::GetInstance().bind("/folder/", "/home/user/docs/"), 0);
}

TEST_F(WebBinderTest, BindBothFiles)
{
    EXPECT_EQ(WebBinder::GetInstance().bind("/file", "/home/user/doc.txt"), 0);
}

TEST_F(WebBinderTest, GetPathExactMatch)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_EQ(WebBinder::GetInstance().getPath("/files"), "/home/user/docs");
}

TEST_F(WebBinderTest, GetPathSubpath)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_EQ(WebBinder::GetInstance().getPath("/files/sub.txt"), "/home/user/docs/sub.txt");
}

TEST_F(WebBinderTest, GetPathEmpty)
{
    EXPECT_EQ(WebBinder::GetInstance().getPath(""), "");
}

TEST_F(WebBinderTest, GetPathNoMatch)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_EQ(WebBinder::GetInstance().getPath("/other"), "");
}

TEST_F(WebBinderTest, ContainWebTrue)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_TRUE(WebBinder::GetInstance().containWeb("/files"));
}

TEST_F(WebBinderTest, ContainWebFalse)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_FALSE(WebBinder::GetInstance().containWeb("/nofile"));
}

TEST_F(WebBinderTest, ContainWebPartial)
{
    WebBinder::GetInstance().bind("/files_data", "/home/user/docs");
    EXPECT_TRUE(WebBinder::GetInstance().containWeb("files"));
}

TEST_F(WebBinderTest, LastWebTrue)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_TRUE(WebBinder::GetInstance().lastWeb("/files"));
}

TEST_F(WebBinderTest, LastWebFalse)
{
    WebBinder::GetInstance().bind("/files", "/home/user/docs");
    EXPECT_FALSE(WebBinder::GetInstance().lastWeb("/other"));
}

TEST_F(WebBinderTest, LastWebEmptyBinds)
{
    EXPECT_FALSE(WebBinder::GetInstance().lastWeb("/files"));
}

TEST_F(WebBinderTest, ClearRemovesAll)
{
    WebBinder::GetInstance().bind("/a", "/home/a");
    WebBinder::GetInstance().bind("/b", "/home/b");
    WebBinder::GetInstance().clear();
    EXPECT_FALSE(WebBinder::GetInstance().containWeb("/a"));
    EXPECT_FALSE(WebBinder::GetInstance().containWeb("/b"));
}

TEST_F(WebBinderTest, MultipleBindsGetPathPicksFirst)
{
    WebBinder::GetInstance().bind("/files", "/home/docs1");
    WebBinder::GetInstance().bind("/files2", "/home/docs2");
    EXPECT_EQ(WebBinder::GetInstance().getPath("/files"), "/home/docs1");
    EXPECT_EQ(WebBinder::GetInstance().getPath("/files2"), "/home/docs2");
}
