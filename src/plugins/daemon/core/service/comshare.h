// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMSHARE_H
#define COMSHARE_H

#include "common/constant.h"
#include "message.pb.h"
#include <co/co.h>
#include <co/json.h>

#include <QList>
#include <QReadWriteLock>
#include <QMap>

typedef enum income_type_t {
    IN_LOGIN_RESULT= 100,
    IN_LOGIN_INFO = 999,
    IN_LOGIN_CONFIRM = 1000,
    IN_TRANSJOB = 1001,
    IN_PEER = 1002,
    FS_ACTION = 1003,
    FS_DATA = 1004,
    FS_DONE = 1005,
    FS_INFO = 1006,
    FS_REPORT = 1007,
    TRANS_CANCEL = 1008,
    TRANS_APPLY = 1009,
    MISC = 1010,
    RPC_PING = 1011,
    TRANS_PAUSE = 1012,
    TRANS_RESUME = 1013,
    APPLY_SHARE_CONNECT = 1014, // 控制方发送请求被控制方连接
    APPLY_SHARE_CONNECT_RES = 1015, // 被控制方回复控制方
    APPLY_SHARE_DISCONNECT = 1016, // 控制方发送请求被控制方连接
    SHARE_START = 1017, // 控制方发送给被控制方，开始共享
    SHARE_START_RES = 1018, // 被控制方通知控制方，共享结果
    SHARE_STOP = 1019, // 停止
    DISCONNECT_CB = 1020, // 断开cb的连接
    DISAPPLY_SHARE_CONNECT = 1021, // 取消申请共享
} IncomeType;

typedef enum outgo_type_t {
    OUT_LOGIN = 2000,
    OUT_TRANSJOB = 2001,
    OUT_PEER = 2002,
    FS_ACTION_RESULT = 2003,
} OutgoType;

struct IncomeData {
    IncomeType type;
    fastring json; // json数据结构实例
    fastring buf; // 二进制数据
};

typedef enum communication_type_t {
    COMM_APPLY_TRANS = 0, // 发送 发送文件请求和回复
} CommunicationType;

struct OutData {
    OutgoType type;
    fastring json; // json数据结构实例
};

extern co::chan<IncomeData> _income_chan;
extern co::chan<OutData> _outgo_chan;

const static QList<uint16> clientPorts{
    7790, 7791
};

typedef enum barrier_type_t {
    Server = 555,
    Client = 666
} BarrierType;

class Comshare
{
private:
    Comshare();
public:
    ~Comshare();
    static Comshare *instance();
    void updateStatus(const CurrentStatus st);
    void updateComdata(const QString &appName, const QString &tarappName, const QString &ip);
    QString targetAppName(const QString &appName);
    QString targetIP(const QString &appName);
    CurrentStatus currentStatus();
    bool checkTransCanConnect();
    bool checkCanTransJob();
private:
    QReadWriteLock _lock;
    std::atomic_int _cur_status {0};// 0是没有连接，1是文件投送连接，
    // 2文件投送申请，3文件发送，4文件接收，5键鼠共享连接，6键鼠共享中
    QMap<QString, QString> _target_app; // appname
    QMap<QString, QString> _target_ip; //
};

#endif // COMSHARE_H
