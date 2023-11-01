// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BRIDGE_H
#define BRIDGE_H

#include <co/fastring.h>

typedef enum req_type_t {
    PING = 10,
    MISC_MSG = 11,
    FRONT_PEER_CB = 100,
    FRONT_CONNECT_CB = 101,
    FRONT_TRANS_STATUS_CB = 102,
    FRONT_FS_PULL_CB = 103,
    FRONT_FS_ACTION_CB = 104,
    FRONT_NOTIFY_FILE_STATUS = 105,
    FRONT_APPLY_TRANS_FILE = 106,
    BACK_GET_DISCOVERY = 200,
    BACK_GET_PEER = 201,
    BACK_GET_PASSWORD = 202,
    BACK_SET_PASSWORD = 203,
    BACK_TRY_CONNECT = 204,
    BACK_GETAPP_CONFIG = 205,
    BACK_SETAPP_CONFIG = 206,
    BACK_TRY_TRANS_FILES = 207,
    BACK_RESUME_JOB = 208,
    BACK_CANCEL_JOB = 209,
    BACK_FS_CREATE = 210,
    BACK_FS_DELETE = 211,
    BACK_FS_RENAME = 212,
    BACK_FS_PULL = 213,
    BACK_DISC_REGISTER = 214,
    BACK_DISC_UNREGISTER = 215,
    BACK_APPLY_TRANS_FILES = 216,
} ReqType;

typedef enum res_type_t {
    FILE_ENTRY = 500,
    FILE_DIRECTORY = 501,
    GENERIC_RESULT = 502,
    FILE_STATUS = 503,
    CALL_RESULT = 504,
    CONFIG_RESULT = 505,
} ResType;

struct BridgeJsonData {
    uint32_t type; // json数据类型：ReqType or ResType
    fastring json; // json数据结构实例
};

#endif // BRIDGE_H
