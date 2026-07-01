// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tokencache.h"
#include "filesystem/path.h"
#include "jwt-cpp/jwt.h"

#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

#include <chrono>
#include <memory>
#include <random>
#include <algorithm>

TokenCache::Es256kKeyPair TokenCache::generateEs256kKeyPair()
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (!ctx) {
        throw std::runtime_error("EVP_PKEY_CTX_new_id failed");
    }
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctxGuard(ctx, EVP_PKEY_CTX_free);

    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        throw std::runtime_error("EVP_PKEY_keygen_init failed");
    }
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_secp256k1) <= 0) {
        throw std::runtime_error("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
    }

    EVP_PKEY *pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0 || !pkey) {
        throw std::runtime_error("EVP_PKEY_keygen failed");
    }
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkeyGuard(pkey, EVP_PKEY_free);

    BIO *bioPriv = BIO_new(BIO_s_mem());
    if (!bioPriv) {
        throw std::runtime_error("BIO_new for private key failed");
    }
    std::unique_ptr<BIO, decltype(&BIO_free)> bioPrivGuard(bioPriv, BIO_free);
    if (PEM_write_bio_PrivateKey(bioPriv, pkey, nullptr, nullptr, 0, nullptr, nullptr) <= 0) {
        throw std::runtime_error("PEM_write_bio_PrivateKey failed");
    }
    char *privData = nullptr;
    long privLen = BIO_get_mem_data(bioPriv, &privData);

    BIO *bioPub = BIO_new(BIO_s_mem());
    if (!bioPub) {
        throw std::runtime_error("BIO_new for public key failed");
    }
    std::unique_ptr<BIO, decltype(&BIO_free)> bioPubGuard(bioPub, BIO_free);
    if (PEM_write_bio_PUBKEY(bioPub, pkey) <= 0) {
        throw std::runtime_error("PEM_write_bio_PUBKEY failed");
    }
    char *pubData = nullptr;
    long pubLen = BIO_get_mem_data(bioPub, &pubData);

    return {std::string(privData, privLen), std::string(pubData, pubLen)};
}

std::string TokenCache::genToken(std::string info)
{
    Es256kKeyPair keyPair;
    try {
        keyPair = generateEs256kKeyPair();
    } catch (const std::exception &ex) {
        std::cout << "Failed to generate ES256k key pair: " << ex.what() << std::endl;
        return "";
    } catch (...) {
        std::cout << "Unknown exception during ES256k key generation" << std::endl;
        return "";
    }

    static thread_local std::mt19937_64 rng(std::random_device{}());
    auto now = std::chrono::system_clock::now();
    auto jti = std::to_string(now.time_since_epoch().count()) + "-" + std::to_string(rng());

    constexpr auto expiry = std::chrono::seconds{36000};

    auto token = jwt::create()
                     .set_issuer("deepin")
                     .set_type("JWT")
                     .set_id(jti)
                     .set_issued_now()
                     .set_expires_in(expiry)
                     .set_payload_claim("web", jwt::claim(info))
                     .sign(jwt::algorithm::es256k(keyPair.publicKey, keyPair.privateKey, "", ""));

    keyPair.expireTime = now + expiry;

    std::lock_guard<std::mutex> lock(_cache_lock);
    cleanupExpiredTokens();

    constexpr size_t MAX_CACHE_SIZE = 600;
    if (_cache.size() >= MAX_CACHE_SIZE) {
        auto oldest = std::min_element(_cache.begin(), _cache.end(),
            [](const auto &a, const auto &b) {
                return a.second.expireTime < b.second.expireTime;
            });
        if (oldest != _cache.end()) {
            _cache.erase(oldest);
        }
    }

    _cache[jti] = std::move(keyPair);

    return token;
}

bool TokenCache::verifyToken(std::string &token)
{
    try {
        auto decoded = jwt::decode(token);

        if (!decoded.has_id()) {
            return false;
        }
        auto jti = decoded.get_id();

        std::lock_guard<std::mutex> lock(_cache_lock);
        cleanupExpiredTokens();
        auto it = _cache.find(jti);
        if (it == _cache.end()) {
            return false;
        }

        auto verifier = jwt::verify()
                              .allow_algorithm(jwt::algorithm::es256k(it->second.publicKey, it->second.privateKey, "", ""))
                              .with_issuer("deepin");

        verifier.verify(decoded);
    } catch (const std::exception& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
        return false;
    } catch (...) {
        return false;
    }

    return true;
}

void TokenCache::cleanupExpiredTokens()
{
    auto now = std::chrono::system_clock::now();
    for (auto it = _cache.begin(); it != _cache.end(); ) {
        if (it->second.expireTime <= now) {
            it = _cache.erase(it);
        } else {
            ++it;
        }
    }
}

void TokenCache::clearTokens()
{
    std::lock_guard<std::mutex> lock(_cache_lock);
    _cache.clear();
}

std::vector<std::string> TokenCache::getWebfromToken(const std::string &token)
{
    auto decoded = jwt::decode(token);
    std::vector<std::string> web_vector;

    try {
        const auto web_array = decoded.get_payload_claim("web").as_string();

        picojson::value v;
        std::string err = picojson::parse(v, web_array);
        if (!err.empty()) {
            std::cout << "json parse error:" << v << std::endl;
            return web_vector;
        }

        for(const auto& name : v.get<picojson::array>()) {
            web_vector.push_back(name.get<std::string>());
        }
    } catch (const std::exception& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
    }
    
    return web_vector;
}
