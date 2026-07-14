// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "datadirectories.h"

using namespace sslconf;

class DataDirectoriesTest : public ::testing::Test {
protected:
    void TearDown() override
    {
        DataDirectories::profile(fs::u8path(""));
        DataDirectories::global(fs::u8path(""));
        DataDirectories::systemconfig(fs::u8path(""));
    }
};

TEST_F(DataDirectoriesTest, ProfileSetAndGet)
{
    auto p = fs::u8path("/tmp/test_profile_1");
    DataDirectories::profile(p);
    EXPECT_EQ(DataDirectories::profile(), p);
}

TEST_F(DataDirectoriesTest, GlobalSetAndGet)
{
    auto p = fs::u8path("/tmp/test_global_1");
    DataDirectories::global(p);
    EXPECT_EQ(DataDirectories::global(), p);
}

TEST_F(DataDirectoriesTest, SystemConfigSetAndGet)
{
    auto p = fs::u8path("/tmp/test_sysconfig_1");
    DataDirectories::systemconfig(p);
    EXPECT_EQ(DataDirectories::systemconfig(), p);
}

TEST_F(DataDirectoriesTest, ProfileDefaultNotEmpty)
{
    DataDirectories::profile(fs::u8path(""));
    auto p = DataDirectories::profile();
    EXPECT_FALSE(p.empty());
}

TEST_F(DataDirectoriesTest, GlobalDefaultNotEmpty)
{
    DataDirectories::global(fs::u8path(""));
    auto p = DataDirectories::global();
    EXPECT_FALSE(p.empty());
}

TEST_F(DataDirectoriesTest, SystemConfigDefaultNotEmpty)
{
    DataDirectories::systemconfig(fs::u8path(""));
    auto p = DataDirectories::systemconfig();
    EXPECT_FALSE(p.empty());
}

TEST_F(DataDirectoriesTest, SslFingerprintsPath)
{
    DataDirectories::profile(fs::u8path("/tmp/fp_base"));
    EXPECT_EQ(DataDirectories::ssl_fingerprints_path().u8string(), "/tmp/fp_base/SSL/Fingerprints");
}

TEST_F(DataDirectoriesTest, LocalSslFingerprintsPath)
{
    DataDirectories::profile(fs::u8path("/tmp/fp_base"));
    EXPECT_EQ(DataDirectories::local_ssl_fingerprints_path().u8string(),
              "/tmp/fp_base/SSL/Fingerprints/Local.txt");
}

TEST_F(DataDirectoriesTest, TrustedServersPath)
{
    DataDirectories::profile(fs::u8path("/tmp/fp_base"));
    EXPECT_EQ(DataDirectories::trusted_servers_ssl_fingerprints_path().u8string(),
              "/tmp/fp_base/SSL/Fingerprints/TrustedServers.txt");
}

TEST_F(DataDirectoriesTest, TrustedClientsPath)
{
    DataDirectories::profile(fs::u8path("/tmp/fp_base"));
    EXPECT_EQ(DataDirectories::trusted_clients_ssl_fingerprints_path().u8string(),
              "/tmp/fp_base/SSL/Fingerprints/TrustedClients.txt");
}

TEST_F(DataDirectoriesTest, SslCertificatePath)
{
    DataDirectories::profile(fs::u8path("/tmp/fp_base"));
    EXPECT_EQ(DataDirectories::ssl_certificate_path().u8string(), "/tmp/fp_base/SSL/Barrier.pem");
}
