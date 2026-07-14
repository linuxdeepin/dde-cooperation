// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "webproto.h"
#include "fileclient.h"
#include "syncstatus.h"

#include "asio/service.h"
#include "asio/ssl_context.h"

#include <filesystem>
#include <fstream>

// InfoEntry serialization round-trip
TEST(WebProtoTest, InfoEntryFileSerialization)
{
    InfoEntry entry;
    entry.name = "test.txt";
    entry.size = 1024;

    auto json = entry.as_json();
    std::string json_str = json.serialize();

    picojson::value v;
    std::string err = picojson::parse(v, json_str);
    EXPECT_TRUE(err.empty());

    InfoEntry decoded;
    decoded.from_json(v);
    EXPECT_EQ(decoded.name, "test.txt");
    EXPECT_EQ(decoded.size, 1024);
    EXPECT_TRUE(decoded.datas.empty());
}

TEST(WebProtoTest, InfoEntryFolderWithChildren)
{
    InfoEntry folder;
    folder.name = "myfolder";
    folder.size = -1;

    InfoEntry child1;
    child1.name = "a.txt";
    child1.size = 100;
    folder.datas.push_back(child1);

    InfoEntry child2;
    child2.name = "b.txt";
    child2.size = 200;
    folder.datas.push_back(child2);

    auto json = folder.as_json().serialize();

    picojson::value v;
    picojson::parse(v, json);
    InfoEntry decoded;
    decoded.from_json(v);

    EXPECT_EQ(decoded.name, "myfolder");
    EXPECT_EQ(decoded.size, -1);
    ASSERT_EQ(decoded.datas.size(), 2u);
    EXPECT_EQ(decoded.datas[0].name, "a.txt");
    EXPECT_EQ(decoded.datas[0].size, 100);
    EXPECT_EQ(decoded.datas[1].name, "b.txt");
    EXPECT_EQ(decoded.datas[1].size, 200);
}

TEST(WebProtoTest, InfoEntryEmptyDatas)
{
    InfoEntry entry;
    entry.name = "empty";
    entry.size = 0;

    auto json = entry.as_json().serialize();
    picojson::value v;
    picojson::parse(v, json);
    InfoEntry decoded;
    decoded.from_json(v);
    EXPECT_EQ(decoded.name, "empty");
    EXPECT_TRUE(decoded.datas.empty());
}

TEST(WebProtoTest, InfoEntryNestedFolders)
{
    InfoEntry root;
    root.name = "root";
    root.size = -1;

    InfoEntry sub;
    sub.name = "sub";
    sub.size = -1;
    InfoEntry leaf;
    leaf.name = "leaf.txt";
    leaf.size = 50;
    sub.datas.push_back(leaf);
    root.datas.push_back(sub);

    auto json = root.as_json().serialize();
    picojson::value v;
    picojson::parse(v, json);
    InfoEntry decoded;
    decoded.from_json(v);

    ASSERT_EQ(decoded.datas.size(), 1u);
    EXPECT_EQ(decoded.datas[0].name, "sub");
    ASSERT_EQ(decoded.datas[0].datas.size(), 1u);
    EXPECT_EQ(decoded.datas[0].datas[0].name, "leaf.txt");
    EXPECT_EQ(decoded.datas[0].datas[0].size, 50);
}

// syncstatus.h enums and constants
TEST(SyncStatusTest, HeaderInfoConstants)
{
    EXPECT_EQ(s_headerInfos[INFO_WEB_START], std::string("webstart"));
    EXPECT_EQ(s_headerInfos[INFO_WEB_FINISH], std::string("webfinish"));
    EXPECT_EQ(s_headerInfos[INFO_WEB_INDEX], std::string("webindex"));
}

TEST(SyncStatusTest, HandleResultValues)
{
    EXPECT_EQ(RES_OKHEADER, 200);
    EXPECT_EQ(RES_NOTFOUND, 404);
    EXPECT_EQ(RES_ERROR, 444);
    EXPECT_EQ(RES_BODY, 555);
    EXPECT_EQ(RES_FINISH, 666);
}

TEST(SyncStatusTest, WebStateValues)
{
    EXPECT_EQ(WEB_DISCONNECTED, -2);
    EXPECT_EQ(WEB_IO_ERROR, -1);
    EXPECT_EQ(WEB_NOT_FOUND, 0);
    EXPECT_EQ(WEB_CONNECTED, 1);
    EXPECT_EQ(WEB_TRANS_START, 2);
    EXPECT_EQ(WEB_TRANS_FINISH, 3);
    EXPECT_EQ(WEB_FILE_BEGIN, 6);
    EXPECT_EQ(WEB_FILE_END, 7);
}
