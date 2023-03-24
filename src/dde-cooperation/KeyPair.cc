// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "KeyPair.h"

#include <iomanip>
#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/stat.h>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <fmt/core.h>

#include <QDebug>

#include <utils/net.h>
#include <utils/ptr.h>

namespace fs = std::filesystem;

KeyPair::KeyPair(const std::filesystem::path &dataDir, KeyType type)
    : m_type(type) {
    std::string typeStr;
    switch (type) {
    case KeyType::ED25519:
        typeStr = "ed25519";
        break;
    }

    auto prikeyFilename = std::string("host_") + typeStr + "_key";
    auto pubkeyFilename = prikeyFilename + ".pub";

    m_privatePath = dataDir / prikeyFilename;
    m_publicPath = dataDir / pubkeyFilename;
}

inline std::string MD5(const std::string &s) {
    auto algo = EVP_md5();
    auto context = make_handle(EVP_MD_CTX_new(), EVP_MD_CTX_free);

    unsigned int hashLen = 0;
    unsigned char hash[EVP_MAX_MD_SIZE];

    EVP_DigestInit_ex(context.get(), algo, nullptr);
    EVP_DigestUpdate(context.get(), s.c_str(), s.size());
    EVP_DigestFinal_ex(context.get(), hash, &hashLen);

    std::stringstream ss;
    for (auto i = 0u; i < hashLen; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << std::uppercase
           << (unsigned int)hash[i];
    }

    return ss.str();
}

bool KeyPair::load() {
    if (!fs::exists(m_privatePath) || !fs::exists(m_publicPath)) {
        if (!generateNewKey()) {
            return false;
        }
    }

    std::ifstream pubf(m_publicPath);
    if (!pubf.is_open()) {
        return false;
    }

    std::string pubkey;
    pubf >> pubkey;

    auto b64 = make_handle(BIO_new(BIO_f_base64()), &BIO_free);
    BIO_set_flags(b64.get(), BIO_FLAGS_BASE64_NO_NL);
    auto bio = make_handle(BIO_new(BIO_s_mem()), &BIO_free);
    BIO_write(bio.get(), pubkey.c_str(), pubkey.size());
    BIO_flush(bio.get());

    auto bioChan = BIO_push(b64.get(), bio.get());

    // base64 encoded 必然比 base64 decoded 长
    int len = BIO_read(bioChan, pubkey.data(), pubkey.size());
    pubkey.resize(len);

    m_fingerprint = MD5(pubkey);

    return true;
}

bool KeyPair::generateNewKey() {
    auto dir = m_privatePath.parent_path();
    if (!fs::exists(dir)) {
        fs::create_directories(dir);
    } else {
        if (!fs::is_directory(dir)) {
            qCritical() << fmt::format("path {} is not a directory", dir.string()).data();
            return false;
        }
    }

    auto ctx = make_handle(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr), &EVP_PKEY_CTX_free);
    if (!ctx) {
        return false;
    }

    if (EVP_PKEY_keygen_init(ctx.get()) <= 0) {
    }

    EVP_PKEY *pkeyTmp = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &pkeyTmp) <= 0) {
        return false;
    }
    auto pkey = make_handle(pkeyTmp, &EVP_PKEY_free);

    // save private key with perm 0600
    auto oldMask = umask(077);
    auto priFp = make_handle(fopen(m_privatePath.c_str(), "w+"), &fclose);
    umask(oldMask);
    if (PEM_write_PrivateKey(priFp.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) <= 0) {
        return false;
    }

    // save public key
    auto pubFbp = make_handle(BIO_new_file(m_publicPath.c_str(), "w+"), &BIO_free);
    auto pubB64bp = make_handle(BIO_new(BIO_f_base64()), &BIO_free);
    BIO_set_flags(pubB64bp.get(), BIO_FLAGS_BASE64_NO_NL);
    auto pubbp = BIO_push(pubB64bp.get(), pubFbp.get());
    if (PEM_write_bio_PUBKEY(pubbp, pkey.get()) <= 0) {
        return false;
    }

    BIO_pop(pubB64bp.get());

    auto hostname = Net::getHostname();
    hostname.insert(0, " ");
    BIO_write(pubFbp.get(), hostname.c_str(), hostname.length());

    return true;
}
