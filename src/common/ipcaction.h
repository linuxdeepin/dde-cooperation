// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IPCACTION_H
#define IPCACTION_H

#define MAX_MQ_LEN 10

typedef enum whoami_t {
    ServiceDeamon = 0,
    AppDataTrans = 1,
    AppCooperation = 2,
    AppSendFile = 3,
} Whoami;

typedef enum ipcevent_t {
    SetConfig = 0,
    ComError = -1,
    Invalid = -2,
    MemLack = -3,
    NoData = -4,
    DataLost = -5
} IpcEvent;

typedef struct
{
    Whoami from;
    Whoami to;
    IpcEvent event;
    char key[128];
    char value[512];
    int spare;
} IpcMessage;


#endif // IPCACTION_H