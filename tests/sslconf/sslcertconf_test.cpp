// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "sslcertconf.h"
#include "datadirectories.h"
#include "filesystem.h"
#include "secureutils.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

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

class SslCertConfTest : public ::testing::Test {
protected:
    std::string tmpDir;
    void SetUp() override
    {
        tmpDir = makeSecureTempDir("sslconf_cert");
        sslconf::fs::create_directories(sslconf::fs::u8path(tmpDir));
    }
    void TearDown() override
    {
        sslconf::fs::remove_all(sslconf::fs::u8path(tmpDir));
        sslconf::DataDirectories::profile(sslconf::fs::u8path(""));
    }
};

TEST_F(SslCertConfTest, InstanceSingleton)
{
    EXPECT_EQ(SslCertConf::ins(), SslCertConf::ins());
}

TEST_F(SslCertConfTest, GenerateCertificate)
{
    auto profile = sslconf::fs::u8path(tmpDir) / "profile1";
    EXPECT_TRUE(SslCertConf::ins()->generateCertificate(profile.u8string()));
    auto certPath = sslconf::DataDirectories::ssl_certificate_path();
    EXPECT_TRUE(sslconf::fs::exists(certPath));
}

TEST_F(SslCertConfTest, GenerateCertificateIdempotent)
{
    auto profile = sslconf::fs::u8path(tmpDir) / "profile2";
    EXPECT_TRUE(SslCertConf::ins()->generateCertificate(profile.u8string()));
    EXPECT_TRUE(SslCertConf::ins()->generateCertificate(profile.u8string()));
    auto fp = SslCertConf::ins()->getFingerPrint();
    EXPECT_FALSE(fp.empty());
}

TEST_F(SslCertConfTest, GetFingerPrintAfterGenerate)
{
    auto profile = sslconf::fs::u8path(tmpDir) / "profile3";
    SslCertConf::ins()->generateCertificate(profile.u8string());
    auto fp = SslCertConf::ins()->getFingerPrint();
    EXPECT_FALSE(fp.empty());
}

TEST_F(SslCertConfTest, GetCertificatePath)
{
    auto profile = sslconf::fs::u8path(tmpDir) / "profile4";
    SslCertConf::ins()->generateCertificate(profile.u8string());
    auto path = SslCertConf::ins()->getCertificatePath();
    EXPECT_FALSE(path.empty());
    EXPECT_NE(path.find("Barrier.pem"), std::string::npos);
}

TEST_F(SslCertConfTest, WriteTrustPrintClient)
{
    auto profile = sslconf::fs::u8path(tmpDir) / "profile5";
    sslconf::DataDirectories::profile(profile);

    std::string print = "v2:sha256:aabbccdd";
    SslCertConf::ins()->writeTrustPrint(false, print);

    auto trustPath = sslconf::DataDirectories::trusted_clients_ssl_fingerprints_path();
    EXPECT_TRUE(sslconf::fs::exists(trustPath));
}

TEST_F(SslCertConfTest, WriteTrustPrintServer)
{
    auto profile = sslconf::fs::u8path(tmpDir) / "profile6";
    sslconf::DataDirectories::profile(profile);

    std::string print = "v2:sha256:eeff0011";
    SslCertConf::ins()->writeTrustPrint(true, print);

    auto trustPath = sslconf::DataDirectories::trusted_servers_ssl_fingerprints_path();
    EXPECT_TRUE(sslconf::fs::exists(trustPath));
}

TEST_F(SslCertConfTest, IsCertificateValidNonExistent)
{
    // Use the private method indirectly: generateCertificate calls is_certificate_valid
    // when the cert file already exists. Test the non-existent case by ensuring
    // generateCertificate succeeds (which means the path doesn't exist and it generates).
    auto profile = sslconf::fs::u8path(tmpDir) / "profile7";
    EXPECT_TRUE(SslCertConf::ins()->generateCertificate(profile.u8string()));
    // Now call again - cert exists, is_certificate_valid is called internally
    EXPECT_TRUE(SslCertConf::ins()->generateCertificate(profile.u8string()));
}

TEST_F(SslCertConfTest, GenerateCertificateInvalidFingerprintPath)
{
    // After generating a valid cert, corrupt it so fingerprint generation fails.
    // Use a profile that generates then we corrupt.
    auto profile = sslconf::fs::u8path(tmpDir) / "profile8";
    SslCertConf::ins()->generateCertificate(profile.u8string());

    auto certPath = sslconf::DataDirectories::ssl_certificate_path();
    // Corrupt the cert file
    {
        std::ofstream out(certPath.native().c_str(), std::ios::trunc);
        out << "corrupted";
    }
    // generateCertificate should now regenerate since is_certificate_valid returns false
    EXPECT_TRUE(SslCertConf::ins()->generateCertificate(profile.u8string()));
}
