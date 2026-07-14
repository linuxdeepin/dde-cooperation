// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "filesystem.h"
#include "finally.h"

#include <fstream>
#include <string>
#include <cstdlib>
#include <cstring>

using namespace sslconf;

static std::string makeSecureTempDir(const char *tmpl)
{
    char buf[256];
    std::snprintf(buf, sizeof(buf), "/tmp/%s_XXXXXX", tmpl);
    char *result = mkdtemp(buf);
    if (!result) {
        throw std::runtime_error(std::string("Failed to create temp dir: ") + tmpl);
    }
    return std::string(result);
}

class FilesystemTest : public ::testing::Test {
protected:
    std::string tmpDir;
    void SetUp() override
    {
        tmpDir = makeSecureTempDir("sslconf_fs");
        fs::create_directories(fs::u8path(tmpDir));
    }
    void TearDown() override
    {
        fs::remove_all(fs::u8path(tmpDir));
    }
};

TEST_F(FilesystemTest, OpenIfstreamRead)
{
    auto path = fs::u8path(tmpDir) / "input.txt";
    {
        std::ofstream out(path.native().c_str());
        out << "hello";
    }
    std::ifstream in;
    open_utf8_path(in, path);
    ASSERT_TRUE(in.good());
    std::string line;
    std::getline(in, line);
    EXPECT_EQ(line, "hello");
}

TEST_F(FilesystemTest, OpenOfstreamWrite)
{
    auto path = fs::u8path(tmpDir) / "output.txt";
    std::ofstream out;
    open_utf8_path(out, path);
    out << "data";
    out.close();

    std::ifstream in(path.native().c_str());
    std::string line;
    std::getline(in, line);
    EXPECT_EQ(line, "data");
}

TEST_F(FilesystemTest, OpenFstreamReadWrite)
{
    auto path = fs::u8path(tmpDir) / "rw.txt";
    {
        std::ofstream init(path.native().c_str());
        init << "12345";
    }
    std::fstream fs;
    open_utf8_path(fs, path);
    fs.seekg(0);
    std::string line;
    std::getline(fs, line);
    EXPECT_EQ(line, "12345");
}

TEST_F(FilesystemTest, FopenUtf8Path)
{
    auto path = fs::u8path(tmpDir) / "cstyle.txt";
    auto fp = fopen_utf8_path(path, "w");
    ASSERT_NE(fp, nullptr);
    std::fputs("test", fp);
    std::fclose(fp);

    fp = fopen_utf8_path(path, "r");
    ASSERT_NE(fp, nullptr);
    char buf[16] = {0};
    std::fgets(buf, sizeof(buf), fp);
    std::fclose(fp);
    EXPECT_STREQ(buf, "test");
}

TEST_F(FilesystemTest, FopenUtf8PathNonExistentRead)
{
    auto path = fs::u8path(tmpDir) / "nofile.txt";
    auto fp = fopen_utf8_path(path, "r");
    EXPECT_EQ(fp, nullptr);
}
