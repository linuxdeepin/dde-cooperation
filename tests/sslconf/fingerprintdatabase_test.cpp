// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "fingerprintdatabase.h"
#include "filesystem.h"

#include <sstream>
#include <cstdlib>
#include <cstdio>

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

class FingerprintDatabaseTest : public ::testing::Test {
protected:
    std::string tmpDir;
    void SetUp() override
    {
        tmpDir = makeSecureTempDir("sslconf_fpdb");
        fs::create_directories(fs::u8path(tmpDir));
    }
    void TearDown() override
    {
        fs::remove_all(fs::u8path(tmpDir));
    }
};

TEST_F(FingerprintDatabaseTest, ParseDbLineV2)
{
    auto fp = FingerprintDatabase::parse_db_line("v2:sha256:aabbcc");
    EXPECT_EQ(fp.algorithm, "sha256");
    ASSERT_EQ(fp.data.size(), 3u);
    EXPECT_EQ(fp.data[0], 0xAA);
}

TEST_F(FingerprintDatabaseTest, ParseDbLineLegacyV1)
{
    std::string line(20, 'a');  // 20 'a' chars = not 40+19, won't match legacy
    auto fp = FingerprintDatabase::parse_db_line(line);
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ParseDbLineLegacyV1Valid)
{
    // legacy: 19 colons, size 40+19 = 59
    std::string line;
    for (int i = 0; i < 20; i++) {
        line += "ab";
        if (i < 19) line += ":";
    }
    auto fp = FingerprintDatabase::parse_db_line(line);
    EXPECT_TRUE(fp.valid());
    EXPECT_EQ(fp.algorithm, "sha1");
}

TEST_F(FingerprintDatabaseTest, ParseDbLineLegacyV1InvalidHex)
{
    // legacy format but invalid hex
    std::string line;
    for (int i = 0; i < 20; i++) {
        line += "zz";
        if (i < 19) line += ":";
    }
    auto fp = FingerprintDatabase::parse_db_line(line);
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ParseDbLineNoVersion)
{
    auto fp = FingerprintDatabase::parse_db_line("nocolon");
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ParseDbLineWrongVersion)
{
    auto fp = FingerprintDatabase::parse_db_line("v1:sha256:aabb");
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ParseDbLineNoAlgo)
{
    auto fp = FingerprintDatabase::parse_db_line("v2:");
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ParseDbLineEmptyData)
{
    auto fp = FingerprintDatabase::parse_db_line("v2:sha256:");
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ParseDbLineInvalidHexData)
{
    auto fp = FingerprintDatabase::parse_db_line("v2:sha256:zz");
    EXPECT_FALSE(fp.valid());
}

TEST_F(FingerprintDatabaseTest, ToDbLine)
{
    FingerprintData fp;
    fp.algorithm = "sha256";
    fp.data = {0xAA, 0xBB};
    auto line = FingerprintDatabase::to_db_line(fp);
    EXPECT_EQ(line, "v2:sha256:aabb");
}

TEST_F(FingerprintDatabaseTest, ReadStreamValid)
{
    std::stringstream ss;
    ss << "v2:sha256:aabbcc\n\n";
    FingerprintDatabase db;
    db.read_stream(ss);
    EXPECT_EQ(db.fingerprints().size(), 1u);
}

TEST_F(FingerprintDatabaseTest, ReadStreamInvalidLines)
{
    std::stringstream ss;
    ss << "invalid\n"
       << "v2:sha256:dd\n"
       << "\n";
    FingerprintDatabase db;
    db.read_stream(ss);
    EXPECT_EQ(db.fingerprints().size(), 1u);
}

TEST_F(FingerprintDatabaseTest, ReadStreamBadStream)
{
    std::stringstream ss;
    ss.setstate(std::ios::badbit);
    FingerprintDatabase db;
    db.read_stream(ss);
    EXPECT_EQ(db.fingerprints().size(), 0u);
}

TEST_F(FingerprintDatabaseTest, WriteStream)
{
    FingerprintDatabase db;
    FingerprintData fp;
    fp.algorithm = "sha256";
    fp.data = {0x01, 0x02};
    db.add_trusted(fp);

    std::ostringstream os;
    db.write_stream(os);
    EXPECT_EQ(os.str(), "v2:sha256:0102\n");
}

TEST_F(FingerprintDatabaseTest, WriteStreamBadStream)
{
    FingerprintDatabase db;
    std::ostringstream os;
    os.setstate(std::ios::badbit);
    db.write_stream(os);
    EXPECT_TRUE(os.str().empty());
}

TEST_F(FingerprintDatabaseTest, AddTrustedAndIsTrusted)
{
    FingerprintDatabase db;
    FingerprintData fp;
    fp.algorithm = "sha1";
    fp.data = {0xAB};

    EXPECT_FALSE(db.is_trusted(fp));
    db.add_trusted(fp);
    EXPECT_TRUE(db.is_trusted(fp));
}

TEST_F(FingerprintDatabaseTest, AddTrustedNoDuplicate)
{
    FingerprintDatabase db;
    FingerprintData fp;
    fp.algorithm = "sha1";
    fp.data = {0xAB};

    db.add_trusted(fp);
    db.add_trusted(fp);
    EXPECT_EQ(db.fingerprints().size(), 1u);
}

TEST_F(FingerprintDatabaseTest, Clear)
{
    FingerprintDatabase db;
    FingerprintData fp;
    fp.algorithm = "sha1";
    fp.data = {0xAB};
    db.add_trusted(fp);
    db.clear();
    EXPECT_EQ(db.fingerprints().size(), 0u);
}

TEST_F(FingerprintDatabaseTest, ReadWriteFile)
{
    auto path = fs::u8path(tmpDir) / "fp.txt";

    FingerprintDatabase db;
    FingerprintData fp;
    fp.algorithm = "sha256";
    fp.data = {0xDE, 0xAD, 0xBE, 0xEF};
    db.add_trusted(fp);
    db.write(path);

    FingerprintDatabase db2;
    db2.read(path);
    EXPECT_EQ(db2.fingerprints().size(), 1u);
    EXPECT_EQ(db2.fingerprints()[0].algorithm, "sha256");
}

TEST_F(FingerprintDatabaseTest, ReadNonExistentFile)
{
    auto path = fs::u8path(tmpDir) / "nonexistent.txt";
    FingerprintDatabase db;
    db.read(path);
    EXPECT_EQ(db.fingerprints().size(), 0u);
}
