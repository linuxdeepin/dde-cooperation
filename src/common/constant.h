// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONSTANT_H
#define CONSTANT_H

#define UNI_KEY "UOS-COOPERATION"
#define UNI_RPC_PORT_UDP  51595
#define UNI_RPC_PORT_BASE (UNI_RPC_PORT_UDP + 2)


#define UNI_IPC_PROTO 1.0
#define UNI_IPC_BACKEND_PORT 7788
#define UNI_IPC_FRONTEND_PORT (UNI_IPC_BACKEND_PORT + 2)

#define BLOCK_SIZE 4 * 1024

const int LOGIN_CONFIRM_TIMEOUT = 30000; // 5 minutes


typedef enum whoami_t {
    ServiceDeamon = 0,
    AppDataTrans = 1,
    AppCooperation = 2,
    AppSendFile = 3,
} Whoami;

typedef enum device_os_t{
    OTHER = 0,
    UOS = 1,
    LINUX = 2,
    WINDOWS = 3,
    MACOS = 4,
    ANDROID = 5,
} DeviceOS;

typedef enum compositor_t {
    CPST_NONE = 0,
    CPST_X11 = 1,
    CPST_WAYLAND = 2,
} Compositor;

typedef enum app_run_t {
    DEEPIN = 0,
    WINE = 1,
} AppRunType;

typedef enum login_result_t {
    TIMEOUT = -2,
    DENY = -1,
    COMING = 0,
    AGREE = 1,
} LoginResult;

typedef enum net_status_t {
    UNKNOWN = -2,
    DISCONNECTED = -1,
    LOSTED = 0,
    CONNECTED = 1,
} NetStatus;

typedef enum peer_result_t {
    LOST= 0,
    ADD = 1,
} PeerResult;

typedef enum do_result_t {
    FAIL= 0,
    SUCCESS = 1,
    DONE = 2,
} DoResult;

typedef enum comm_type_t {
    LOGIN = 0, // result: LoginResult
    NET_STATUS = 1, // result: NetStatus
    PEER = 2, // result: PeerResult
    APP_INSTALL = 4, // result: DoResult
    WEB_IMPORT = 5, // result: DoResult
} CommType;

typedef enum fs_type_t {
    TRANS_SPEED = 0,
    TRANS_SEND = 1,
    TRANS_RECV = 2,
    ACTION_READ = 3,
    ACTION_REMOVE = 4,
    ACTION_CREATE = 5,
    ACTION_RENAME = 6,
    JOB_RESUME = 7,
    JOB_CANCEL = 8,
    JOB_DONE = 9,
} FSType;

typedef enum flow_type_t {
    TRANS_BLOCK = 0,
    TRANS_DIGEST = 1,
    TRANS_ERROR = 2,
    TRANS_DONE = 3,
} FSFlowType;

typedef enum rpc_result_t {
    PARAM_ERROR = -2,
    INVOKE_FAIL = -1,
    INVOKE_OK = 0,
    INVOKE_DONE = 1,
} RpcResult;


#endif // CONSTANT_H
