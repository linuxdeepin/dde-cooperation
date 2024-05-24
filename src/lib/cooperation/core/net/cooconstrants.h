// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COO_CONSTRANTS_H
#define COO_CONSTRANTS_H

#define COO_SESSION_PORT 51566
#define COO_HARD_PIN "515616"

#define COO_WEB_PORT 51568

typedef enum apply_type_t {
    APPLY_INFO = 100,   // 设备信息申请
    APPLY_TRANS = 101,   // 传输申请
    APPLY_TRANS_RESULT = 102,   // 传输申请的结果
    APPLY_SHARE = 111,   // 控制申请
    APPLY_SHARE_RESULT = 112,   // 控制申请的结果
    APPLY_SHARE_STOP = 113,   // 收到停止事件
    APPLY_CANCELED = 120,   // 申请被取消
} ApplyReqType;

#endif   // COO_CONSTRANTS_H
