// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "include/sslcertconf.h"
#include "datadirectories.h"
#include "finally.h"
#include "filesystem.h"
#include "fingerprintdatabase.h"
#include "secureutils.h"

#include <iostream>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

SslCertConf *SslCertConf::ins()
{
    static SslCertConf ins;
    return &ins;
}

bool SslCertConf::generateCertificate(const std::string &profile)
{
    // first set the default profile
    auto profile_path = sslconf::fs::u8path(profile.c_str());
    sslconf::DataDirectories::profile(profile_path);

    // check and create a self sign cert
    auto cert_path = sslconf::DataDirectories::ssl_certificate_path();
    if (!sslconf::fs::exists(cert_path) || !is_certificate_valid(cert_path)) {
        try {
            auto cert_dir = cert_path.parent_path();
            if (!sslconf::fs::exists(cert_dir)) {
                sslconf::fs::create_directories(cert_dir);
            }

            sslconf::generate_pem_self_signed_cert(cert_path.u8string());
        }  catch (const std::exception &e) {
            std::cout << "SSL tool failed: " << e.what() << std::endl;
            return false;
        }

        //std::cout << "SSL certificate generated." << std::endl;
    }

    // gen the fingerprint by cert.
    return generate_fingerprint(cert_path);
}

std::string SslCertConf::getFingerPrint()
{
    return _fingerPrint;
}

void SslCertConf::writeTrustPrint(bool server, const std::string &print)
{
    auto trust_path = sslconf::DataDirectories::local_ssl_fingerprints_path(); //default: Local
    if (server) {
        trust_path = sslconf::DataDirectories::trusted_servers_ssl_fingerprints_path();
    } else {
        trust_path = sslconf::DataDirectories::trusted_clients_ssl_fingerprints_path();
    }
    auto trust_dir = trust_path.parent_path();
    if (!sslconf::fs::exists(trust_dir)) {
        sslconf::fs::create_directories(trust_dir);
    }

    sslconf::FingerprintDatabase db;
    auto sha256 = db.parse_db_line(print);
    db.add_trusted(sha256);
    db.write(trust_path);
}

SslCertConf::SslCertConf()
{
}

bool SslCertConf::generate_fingerprint(const gfs::path &cert_path)
{
    try {
        sslconf::FingerprintDatabase db;
        auto sha256 = sslconf::get_pem_file_cert_fingerprint(cert_path.u8string(),
                                                             sslconf::FingerprintType::SHA256);

        _fingerPrint = db.to_db_line(sha256);

        //std::cout << "SSL fingerprint generated: " << _fingerPrint << std::endl;

    } catch (const std::exception &e) {
        std::cout << "Failed to find SSL fingerprint. " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool SslCertConf::is_certificate_valid(const gfs::path &path)
{
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    auto fp = sslconf::fopen_utf8_path(path, "r");
    if (!fp) {
        std::cout << "Could not read from default certificate file." << std::endl;
        return false;
    }
    auto file_close = sslconf::finally([fp]() { std::fclose(fp); });

    auto *cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert) {
        std::cout << "Error loading default certificate file to memory." << std::endl;
        return false;
    }
    auto cert_free = sslconf::finally([cert]() { X509_free(cert); });

    auto *pubkey = X509_get_pubkey(cert);
    if (!pubkey) {
        std::cout << "Default certificate key file does not contain valid public key" << std::endl;
        return false;
    }
    auto pubkey_free = sslconf::finally([pubkey]() { EVP_PKEY_free(pubkey); });

    auto type = EVP_PKEY_type(EVP_PKEY_id(pubkey));
    if (type != EVP_PKEY_RSA && type != EVP_PKEY_DSA) {
        std::cout << "Public key in default certificate key file is not RSA or DSA" << std::endl;
        return false;
    }

    auto bits = EVP_PKEY_bits(pubkey);
    if (bits < 2048) {
        // We could have small keys in old installations
        std::cout << "Public key in default certificate key file is too small." << std::endl;
        return false;
    }

    return true;
}
