// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SESSIONPROTO_H
#define SESSIONPROTO_H

#include <stdlib.h>
#include <string.h>
#include <memory>
#include <functional>

#ifndef PICOJSON_USE_INT64
#define PICOJSON_USE_INT64
#endif
#include "picojson/picojson.h"


#define SESSION_TCP_PORT  51616
#define WEB_TCP_PORT  SESSION_TCP_PORT + 2

enum LoginStatus {
    LOGIN_SUCCESS,
    LOGIN_DENIED
};

enum CallResult {
    DO_SUCCESS,
    DO_FAILED
};

typedef enum dt_type_t {
    REQ_LOGIN = 1000,
    REQ_FREE_SPACE = 1001,
    REQ_TRANS_DATAS = 1002,
    REQ_TRANS_CANCLE = 1003,
    CAST_INFO = 1004,
} ComType;

typedef enum apply_flag_t {
    ASK_NEEDCONFIRM = 10,
    ASK_QUIET = 12,
    ASK_CANCELED = 14,
    DO_WAIT = 20,
    DO_DONE = 22,
    REPLY_ACCEPT = 30,
    REPLY_REJECT = 32,
} ApplyFlag;

//misc:
//unfinish_json
//remaining_space
//add_result
//change_page
//transfer_content

using ExtenMessageHandler = std::function<bool(int32_t mask, const picojson::value &json_value, std::string *res_msg)>;

struct LoginMessage {
    std::string name;
    std::string auth;

    void from_json(const picojson::value& _x_) {
        name = _x_.get("name").to_str();
        auth = _x_.get("auth").to_str();
    }

    picojson::value as_json() const {
        picojson::object obj;
        obj["name"] = picojson::value(name);
        obj["auth"] = picojson::value(auth);
        return picojson::value(obj);
    }
};

struct FreeSpaceMessage {
    int64_t total;
    int64_t free;

    void from_json(const picojson::value& _x_) {
        total = _x_.get("total").get<int64_t>();
        free = _x_.get("free").get<int64_t>();
    }

    picojson::value as_json() const {
        picojson::object obj;
        obj["total"] = picojson::value(total);
        obj["free"] = picojson::value(free);
        return picojson::value(obj);
    }
};

struct TransDataMessage {
    std::string id; // the file serve id.
    std::vector<std::string> names; // the file/dir name list
    std::string endpoint; // ip:port:token
    bool flag;  // request: dir or file;  response: ready to receive or error(not enough space)
    int64_t size;  // the space need, total folder size or all file size

    void from_json(const picojson::value& _x_) {
        id = _x_.get("id").to_str();
//        names = _x_.get("names").to_str();
        endpoint = _x_.get("token").to_str();
        flag = _x_.get("flag").get<bool>();
        size = _x_.get("size").get<int64_t>();

        if (_x_.get("names").is<picojson::array>()) {
            const picojson::array &namesArr = _x_.get("names").get<picojson::array>();
            for (const auto &elem : namesArr) {
                if (elem.is<std::string>()) {
                    names.push_back(elem.get<std::string>());
                }
            }
        }
    }

    picojson::value as_json() const {
        picojson::object obj;
        obj["id"] = picojson::value(id);
        obj["token"] = picojson::value(endpoint);
        obj["flag"] = picojson::value(flag);
        obj["size"] = picojson::value(size);

        picojson::array namesArr;
        for (const auto &name : names) {
            namesArr.push_back(picojson::value(name));
        }
        obj["names"] = picojson::value(namesArr);
        return picojson::value(obj);
    }
};

struct TransCancelMessage {
    std::string id; // the file serve id.
    std::string name; // the file/dir name
    std::string reason; // user or io error

    void from_json(const picojson::value& _x_) {
        id = _x_.get("id").to_str();
        name = _x_.get("name").to_str();
        reason = _x_.get("reason").to_str();
    }

    picojson::value as_json() const {
        picojson::object obj;
        obj["id"] = picojson::value(id);
        obj["name"] = picojson::value(name);
        obj["reason"] = picojson::value(reason);
        return picojson::value(obj);
    }
};

struct ApplyMessage {
    int64_t flag {0};
    std::string  nick;
    std::string  host;
    int64_t port {0};

    void from_json(const picojson::value& _x_) {
        flag = _x_.get("flag").get<int64_t>();
        nick = _x_.get("nick").to_str();
        host = _x_.get("selfIp").to_str();
        port = _x_.get("selfPort").get<int64_t>();
    }

    picojson::value as_json() const {
        picojson::object obj;
        obj["flag"] = picojson::value(flag);
        obj["nick"] = picojson::value(nick);
        obj["selfIp"] = picojson::value(host);
        obj["selfPort"] = picojson::value(port);
        return picojson::value(obj);
    }
};

struct MyInfoMessage {
    std::string nickname;
    std::string username;
    std::string hostname;
    std::string ipv4;
    std::string port;

    void from_json(const picojson::value& _x_) {
        nickname = _x_.get("nickname").to_str();
        username = _x_.get("username").to_str();
        hostname = _x_.get("hostname").to_str();
        ipv4 = _x_.get("ipv4").to_str();
        port = _x_.get("port").to_str();
    }

    picojson::value as_json() const {
        picojson::object obj;
        obj["nickname"] = picojson::value(nickname);
        obj["username"] = picojson::value(username);
        obj["hostname"] = picojson::value(hostname);
        obj["ipv4"] = picojson::value(ipv4);
        obj["port"] = picojson::value(port);
        return picojson::value(obj);
    }
};

// struct ShareServerConfig {
//     fastring server_screen{ "" };
//     fastring client_screen{ "" };
//     fastring screen_left{ "" };
//     fastring screen_right{ "" };
//     bool left_halfDuplexCapsLock{ false };
//     bool left_halfDuplexNumLock{ false };
//     bool left_halfDuplexScrollLock{ false };
//     bool left_xtestIsXineramaUnaware{ false };
//     bool left_preserveFocus{ false };
//     fastring left_switchCorners{ "none" };
//     int32 left_switchCornerSize{ 0 };
//     bool right_halfDuplexCapsLock{ false };
//     bool right_halfDuplexNumLock{ false };
//     bool right_halfDuplexScrollLock{ false };
//     bool right_xtestIsXineramaUnaware{ false };
//     bool right_preserveFocus{ false };
//     fastring right_switchCorners{ "none" };
//     int32 right_switchCornerSize{ 0 };
//     bool relativeMouseMoves{ false };
//     bool screenSaverSync{ true };
//     bool win32KeepForeground{ false };
//     bool clipboardSharing{ false };
//     fastring switchCorners{ "none" };
//     int32 switchCornerSize{ 0 };

//     void from_json(const co::Json& _x_) {
//         server_screen = _x_.get("server_screen").as_c_str();
//         client_screen = _x_.get("client_screen").as_c_str();
//         screen_left = _x_.get("screen_left").as_c_str();
//         screen_right = _x_.get("screen_right").as_c_str();
//         left_halfDuplexCapsLock = _x_.get("left_halfDuplexCapsLock").as_bool();
//         left_halfDuplexNumLock = _x_.get("left_halfDuplexNumLock").as_bool();
//         left_halfDuplexScrollLock = _x_.get("left_halfDuplexScrollLock").as_bool();
//         left_xtestIsXineramaUnaware = _x_.get("left_xtestIsXineramaUnaware").as_bool();
//         left_preserveFocus = _x_.get("left_preserveFocus").as_bool();
//         left_switchCorners = _x_.get("left_switchCorners").as_c_str();
//         left_switchCornerSize = (int32)_x_.get("left_switchCornerSize").as_int64();
//         right_halfDuplexCapsLock = _x_.get("right_halfDuplexCapsLock").as_bool();
//         right_halfDuplexNumLock = _x_.get("right_halfDuplexNumLock").as_bool();
//         right_halfDuplexScrollLock = _x_.get("right_halfDuplexScrollLock").as_bool();
//         right_xtestIsXineramaUnaware = _x_.get("right_xtestIsXineramaUnaware").as_bool();
//         right_preserveFocus = _x_.get("right_preserveFocus").as_bool();
//         right_switchCorners = _x_.get("right_switchCorners").as_c_str();
//         right_switchCornerSize = (int32)_x_.get("right_switchCornerSize").as_int64();
//         relativeMouseMoves = _x_.get("relativeMouseMoves").as_bool();
//         screenSaverSync = _x_.get("screenSaverSync").as_bool();
//         win32KeepForeground = _x_.get("win32KeepForeground").as_bool();
//         clipboardSharing = _x_.get("clipboardSharing").as_bool();
//         switchCorners = _x_.get("switchCorners").as_c_str();
//         switchCornerSize = (int32)_x_.get("switchCornerSize").as_int64();
//     }

//     co::Json as_json() const {
//         co::Json _x_;
//         _x_.add_member("server_screen", server_screen);
//         _x_.add_member("client_screen", client_screen);
//         _x_.add_member("screen_left", screen_left);
//         _x_.add_member("screen_right", screen_right);
//         _x_.add_member("left_halfDuplexCapsLock", left_halfDuplexCapsLock);
//         _x_.add_member("left_halfDuplexNumLock", left_halfDuplexNumLock);
//         _x_.add_member("left_halfDuplexScrollLock", left_halfDuplexScrollLock);
//         _x_.add_member("left_xtestIsXineramaUnaware", left_xtestIsXineramaUnaware);
//         _x_.add_member("left_preserveFocus", left_preserveFocus);
//         _x_.add_member("left_switchCorners", left_switchCorners);
//         _x_.add_member("left_switchCornerSize", left_switchCornerSize);
//         _x_.add_member("right_halfDuplexCapsLock", right_halfDuplexCapsLock);
//         _x_.add_member("right_halfDuplexNumLock", right_halfDuplexNumLock);
//         _x_.add_member("right_halfDuplexScrollLock", right_halfDuplexScrollLock);
//         _x_.add_member("right_xtestIsXineramaUnaware", right_xtestIsXineramaUnaware);
//         _x_.add_member("right_preserveFocus", right_preserveFocus);
//         _x_.add_member("right_switchCorners", right_switchCorners);
//         _x_.add_member("right_switchCornerSize", right_switchCornerSize);
//         _x_.add_member("relativeMouseMoves", relativeMouseMoves);
//         _x_.add_member("screenSaverSync", screenSaverSync);
//         _x_.add_member("win32KeepForeground", win32KeepForeground);
//         _x_.add_member("clipboardSharing", clipboardSharing);
//         _x_.add_member("switchCorners", switchCorners);
//         _x_.add_member("switchCornerSize", switchCornerSize);
//         return _x_;
//     }
// };

#endif // SESSIONPROTO_H
