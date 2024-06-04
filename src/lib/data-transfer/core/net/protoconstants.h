// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROTOCONSTANTS_H
#define PROTOCONSTANTS_H

#define DATA_SESSION_PORT 51616
#define DATA_HARD_PIN "515616"

#define DATA_WEB_PORT 51588

typedef enum session_type_t {
    SESSION_LOGIN = 101,   // 登录
    SESSION_TRANS = 102,   // 传输
    SESSION_STOP = 111,   // 中断传输
    SESSION_MESSAGE = 112,   //字符串信息
} SessionReqType;

#endif   // PROTOCONSTANTS_H
