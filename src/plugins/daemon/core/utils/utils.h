// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CORE_UTIL_H
#define CORE_UTIL_H

#include <stdint.h>
#include <string.h>
#include <regex>
#include "co/rand.h"
#include "co/hash.h"
#include "co/str.h"

#ifdef __linux__
#include <unistd.h>
#include <uuid/uuid.h>
#include <pwd.h>
#elif _WIN32
#include <windows.h>
#include <objbase.h>
#include <Lmcons.h>
#else
#error "Unsupported platform"
#endif


class Util {
public:

    static std::string genRandPin()
    {
        int pin_len = 6;
        return std::string(co::randstr("0123456789", pin_len).c_str());
    }

    static std::string genAuthToken(const char *uuid, const char *pin)
    {
        std::string all = uuid;// + pin;
        fastring encodes = base64_encode(all);
        return std::string(encodes.c_str());
    }

    static bool checkToken(const char *token)
    {
        return true;
    }

    static std::string decodeBase64(const char *str)
    {
        fastring decodes = base64_decode(str);
        return std::string(decodes.c_str());
    }

    static std::string encodeBase64(const char *str)
    {
        fastring encodes = base64_encode(str);
        return std::string(encodes.c_str());
    }

    static std::string getHostname(void)
    {
#ifdef __linux__
        char hostName[_SC_HOST_NAME_MAX];
        gethostname(hostName, _SC_HOST_NAME_MAX);
        return hostName;
#else 
        char hostname[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(hostname) / sizeof(hostname[0]);
        if (GetComputerNameA(hostname, &size)) {
            return std::string(hostname);
        }
        return "";
#endif
    }

    static std::string getCurrentUsername()
    {
#ifdef __linux__
        uid_t uid = geteuid();
        struct passwd *pw = getpwuid(uid);
        if (pw != nullptr) {
            return std::string(pw->pw_name);
        }
        return "";
#else 
        TCHAR username[UNLEN + 1];
        DWORD usernameLen = UNLEN + 1;
        GetUserName(username, &usernameLen);
        return std::string(username);
#endif
    }

    static std::string genUUID()
    {
#ifdef __linux__
        uuid_t uuid;
        uuid_generate(uuid);
        char uuidStr[100];
        uuid_unparse(uuid, uuidStr);

        return uuidStr;
#else
        UUID uuid;
        UuidCreate(&uuid);

        RPC_CSTR uuidStr;
        UuidToStringA(&uuid, &uuidStr);

        std::string uuidString(reinterpret_cast<char *>(uuidStr));

        RpcStringFreeA(&uuidStr);

        return uuidString;
#endif
    }

    static bool isValidUUID(const std::string &uuid)
    {
        // UUID正则表达式模式
        std::regex pattern(R"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})");

        // 使用正则表达式匹配UUID
        return std::regex_match(uuid, pattern);
    }

    static std::string parseFileName(const char *path)
    {
        fastring file_name;
        co::vector<fastring> path_slips = str::split(path, '/');
        file_name = path_slips.pop_back();
        
        return std::string(file_name.c_str());
    }

};

#endif
