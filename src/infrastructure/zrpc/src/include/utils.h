// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZRPC_UTILS_H
#define ZRPC_UTILS_H

#include <string>
#include <cstdint>
#include <random>

namespace zrpc_ns {

class Util {
public:
    static uint32_t netByteToInt32(const char *buf) {
        uint32_t value = 0;
        value = ((uint32_t)(buf[0] & 0xff) << 24) |
                ((uint32_t)(buf[1] & 0xff) << 16) |
                ((uint32_t)(buf[2] & 0xff) << 8) |
                ((uint32_t)(buf[3] & 0xff));
        return value;
    }

    static std::string genMsgNumber() {
        static const int t_msg_req_len = 20;
        static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

        std::string str;
        str.reserve(t_msg_req_len);
        for (int i = 0; i < t_msg_req_len; ++i) {
            str += charset[dis(gen)];
        }
        return str;
    }
};

} // namespace zrpc_ns

#endif
