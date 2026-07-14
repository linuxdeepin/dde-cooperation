// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "secureutils.h"
#include "filesystem.h"

#include <openssl/x509.h>
#include <openssl/pem.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

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

class SecureUtilsTest : public ::testing::Test {
protected:
    std::string tmpDir;
    void SetUp() override
    {
        tmpDir = makeSecureTempDir("sslconf_sec");
        fs::create_directories(fs::u8path(tmpDir));
    }
    void TearDown() override
    {
        fs::remove_all(fs::u8path(tmpDir));
    }
};

// ---- pure formatting functions ----

TEST_F(SecureUtilsTest, FormatSslFingerprintWithSeparator)
{
    std::vector<uint8_t> fp = {0xAB, 0xCD, 0xEF, 0x01};
    auto r = format_ssl_fingerprint(fp, true);
    EXPECT_EQ(r, "AB:CD:EF:01");
}

TEST_F(SecureUtilsTest, FormatSslFingerprintNoSeparator)
{
    std::vector<uint8_t> fp = {0xAB, 0xCD};
    auto r = format_ssl_fingerprint(fp, false);
    EXPECT_EQ(r, "ABCD");
}

TEST_F(SecureUtilsTest, FormatSslFingerprintEmpty)
{
    std::vector<uint8_t> fp;
    auto r = format_ssl_fingerprint(fp, true);
    EXPECT_TRUE(r.empty());
}

TEST_F(SecureUtilsTest, FormatSslFingerprintSingleByte)
{
    std::vector<uint8_t> fp = {0x0A};
    auto r = format_ssl_fingerprint(fp, true);
    EXPECT_EQ(r, "0A");
}

TEST_F(SecureUtilsTest, FormatSslFingerprintColumns)
{
    std::vector<uint8_t> fp = {0xAB, 0xCD, 0xEF, 0x01};
    auto r = format_ssl_fingerprint_columns(fp);
    EXPECT_FALSE(r.empty());
    EXPECT_NE(r.find("AB"), std::string::npos);
    EXPECT_NE(r.find("CD"), std::string::npos);
    EXPECT_NE(r.find(':'), std::string::npos);
}

TEST_F(SecureUtilsTest, FormatSslFingerprintColumnsEmpty)
{
    std::vector<uint8_t> fp;
    auto r = format_ssl_fingerprint_columns(fp);
    EXPECT_TRUE(r.empty());
}

TEST_F(SecureUtilsTest, FormatSslFingerprintColumnsOddLength)
{
    // odd number of hex chars when size is odd byte count is impossible with bytes,
    // but to_hex always produces even. Test the empty/normal cases.
    std::vector<uint8_t> fp = {0x1};
    auto r = format_ssl_fingerprint_columns(fp);
    EXPECT_FALSE(r.empty());
}

TEST_F(SecureUtilsTest, CreateFingerprintRandomart)
{
    std::vector<uint8_t> digest = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    auto art = create_fingerprint_randomart(digest);
    EXPECT_FALSE(art.empty());
    // art starts with '+'---...---'+'\n
    EXPECT_EQ(art[0], '+');
    EXPECT_NE(art.find('\n'), std::string::npos);
}

TEST_F(SecureUtilsTest, CreateFingerprintRandomartEmpty)
{
    std::vector<uint8_t> digest;
    auto art = create_fingerprint_randomart(digest);
    EXPECT_FALSE(art.empty());
    EXPECT_EQ(art[0], '+');
}

TEST_F(SecureUtilsTest, GetDigestByTypeSha1)
{
    // getDigestbyType is internal; test indirectly via get_ssl_cert_fingerprint with SHA1
    auto path = fs::u8path(tmpDir) / "digest_sha1.pem";
    generate_pem_self_signed_cert(path.u8string());
    auto fp = get_pem_file_cert_fingerprint(path.u8string(), FingerprintType::SHA1);
    EXPECT_EQ(fp.algorithm, "sha1");
    EXPECT_FALSE(fp.data.empty());
}

TEST_F(SecureUtilsTest, GetDigestByTypeSha256)
{
    // getDigestbyType is internal; test indirectly via get_ssl_cert_fingerprint with SHA256
    auto path = fs::u8path(tmpDir) / "digest_sha256.pem";
    generate_pem_self_signed_cert(path.u8string());
    auto fp = get_pem_file_cert_fingerprint(path.u8string(), FingerprintType::SHA256);
    EXPECT_EQ(fp.algorithm, "sha256");
    EXPECT_FALSE(fp.data.empty());
}

TEST_F(SecureUtilsTest, GetDigestByTypeInvalid)
{
    // INVALID type throws via getDigestbyType; test indirectly through get_ssl_cert_fingerprint
    auto path = fs::u8path(tmpDir) / "cert_invalid_type.pem";
    generate_pem_self_signed_cert(path.u8string());
    auto fp_in = fopen_utf8_path(path, "r");
    ASSERT_NE(fp_in, nullptr);
    auto cert = PEM_read_X509(fp_in, nullptr, nullptr, nullptr);
    std::fclose(fp_in);
    ASSERT_NE(cert, nullptr);
    EXPECT_THROW(get_ssl_cert_fingerprint(cert, FingerprintType::INVALID), std::runtime_error);
    X509_free(cert);
}

// ---- cert generation + fingerprint extraction (real OpenSSL) ----

TEST_F(SecureUtilsTest, GeneratePemSelfSignedCert)
{
    auto path = fs::u8path(tmpDir) / "test_cert.pem";
    generate_pem_self_signed_cert(path.u8string());
    EXPECT_TRUE(fs::exists(path));
}

TEST_F(SecureUtilsTest, GetPemFileCertFingerprint)
{
    auto path = fs::u8path(tmpDir) / "cert_fp.pem";
    generate_pem_self_signed_cert(path.u8string());

    auto fp = get_pem_file_cert_fingerprint(path.u8string(), FingerprintType::SHA256);
    EXPECT_EQ(fp.algorithm, "sha256");
    EXPECT_FALSE(fp.data.empty());
}

TEST_F(SecureUtilsTest, GetPemFileCertFingerprintNonExistent)
{
    EXPECT_THROW(get_pem_file_cert_fingerprint("/tmp/nonexistent_cert.pem", FingerprintType::SHA256),
                 std::runtime_error);
}

TEST_F(SecureUtilsTest, GetPemFileCertFingerprintInvalid)
{
    auto path = fs::u8path(tmpDir) / "bad.pem";
    {
        std::ofstream out(path.native().c_str());
        out << "not a cert";
    }
    EXPECT_THROW(get_pem_file_cert_fingerprint(path.u8string(), FingerprintType::SHA256),
                 std::runtime_error);
}

TEST_F(SecureUtilsTest, GetSslCertFingerprintNull)
{
    EXPECT_THROW(get_ssl_cert_fingerprint(nullptr, FingerprintType::SHA256), std::runtime_error);
}

TEST_F(SecureUtilsTest, GetSslCertFingerprintValid)
{
    auto path = fs::u8path(tmpDir) / "cert_valid.pem";
    generate_pem_self_signed_cert(path.u8string());

    auto fp_in = fopen_utf8_path(path, "r");
    ASSERT_NE(fp_in, nullptr);
    auto cert = PEM_read_X509(fp_in, nullptr, nullptr, nullptr);
    std::fclose(fp_in);
    ASSERT_NE(cert, nullptr);

    auto fp = get_ssl_cert_fingerprint(cert, FingerprintType::SHA1);
    EXPECT_EQ(fp.algorithm, "sha1");
    EXPECT_FALSE(fp.data.empty());
    X509_free(cert);
}

TEST_F(SecureUtilsTest, GenerateCertFailsOnBadPath)
{
    EXPECT_THROW(generate_pem_self_signed_cert("/nonexistent_dir/sub/cert.pem"),
                 std::runtime_error);
}
