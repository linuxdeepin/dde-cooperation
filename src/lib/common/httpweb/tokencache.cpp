// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tokencache.h"
#include "filesystem/path.h"
#include "jwt-cpp/jwt.h"

#include "configs/crypt/cert.h"

std::string TokenCache::getAllCache()
{
    std::scoped_lock locker(_cache_lock);
    std::string result;
    result += "[\n";
    for (const auto &item : _cache) {
        result += "  {\n";
        result += "    \"key\": \"" + item.first + "\",\n";
        result += "    \"value\": \"" + item.second + "\",\n";
        result += "  },\n";
    }
    result += "]\n";
    return result;
}

bool TokenCache::getCacheValue(std::string_view key, std::string &value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.find(key);
    if (it != _cache.end()) {
        value = it->second;
        return true;
    } else
        return false;
}

void TokenCache::putCacheValue(std::string_view key, std::string_view value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.emplace(key, value);
    if (!it.second)
        it.first->second = value;
}

bool TokenCache::deleteCacheValue(std::string_view key, std::string &value)
{
    std::scoped_lock locker(_cache_lock);
    auto it = _cache.find(key);
    if (it != _cache.end()) {
        value = it->second;
        _cache.erase(it);
        return true;
    } else
        return false;
}

void TokenCache::clearCache()
{
    _cache.clear();
}

std::string TokenCache::genToken(std::string info)
{
    const std::string es256k_priv_key = Cert::instance()->getPriEs256k();
    const std::string es256k_pub_key = Cert::instance()->getPubEs256k();

    auto token = jwt::create()
                     .set_issuer("deepin")
                     .set_type("JWT")
                     .set_id("dde-cooperation")
                     .set_issued_now()
                     .set_expires_in(std::chrono::seconds{36000})
                     .set_payload_claim("web", jwt::claim(info))
                     .sign(jwt::algorithm::es256k(es256k_pub_key, es256k_priv_key, "", ""));

    return token;
}

bool TokenCache::verifyToken(std::string &token)
{
    const std::string es256k_priv_key = Cert::instance()->getPriEs256k();
    const std::string es256k_pub_key = Cert::instance()->getPubEs256k();
    auto verifier = jwt::verify()
                          .allow_algorithm(jwt::algorithm::es256k(es256k_pub_key, es256k_priv_key, "", ""))
                          .with_issuer("deepin");

    auto decoded = jwt::decode(token);

    try {
        verifier.verify(decoded);
        std::cout << "Token verify success!" << std::endl;
    } catch (const std::exception& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
        return false;
    }

    return true;
}

std::vector<std::string> TokenCache::getWebfromToken(const std::string &token)
{
    auto decoded = jwt::decode(token);
    std::vector<std::string> web_vector;

    try {
        const auto web_array = decoded.get_payload_claim("web").as_string();
        std::cout << "web = " << web_array << std::endl;

        picojson::value v;
        std::string err = picojson::parse(v, web_array);
        if (!err.empty()) {
            std::cout << "json parse error:" << v << std::endl;
            return web_vector;
        }

        // Assuming web_array is an array of strings
        for(const auto& name : v.get<picojson::array>()) {
            web_vector.push_back(name.get<std::string>());
            std::cout << "Name: " << name.get<std::string>() << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cout << "Error: " << ex.what() << std::endl;
    }
    
    return web_vector;
}

//bool TokenCache::setMountPoint(const std::string &mount_point, const std::string &dir)
//{
//    CppCommon::Path dirpath = dir;
//    if (dirpath.IsDirectory()) {
//        std::string mnt = !mount_point.empty() ? mount_point : "/";
//        if (!mnt.empty() && mnt[0] == '/') {
//            _base_dirs.push_back({mnt, dir});
//            return true;
//        }
//    }
//    return false;
//}

//bool TokenCache::removeMountPoint(const std::string &mount_point)
//{
//    for (auto it = _base_dirs.begin(); it != _base_dirs.end(); ++it) {
//        if (it->mount_point == mount_point) {
//            _base_dirs.erase(it);
//            return true;
//        }
//    }
//    return false;
//}

//std::vector<MountPointEntry> TokenCache::getBaseDir()
//{
//    return _base_dirs;
//}
