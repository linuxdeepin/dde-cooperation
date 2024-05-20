// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SYNCSTATUS_H
#define SYNCSTATUS_H

#include <memory>
#include <functional>
#include <string>

enum WebState {
    WEB_ERROR = -2,
    WEB_DISCONNECTED = -1,
    WEB_DISCONNECTING = 0,
    WEB_CONNECTING = 1,
    WEB_CONNECTED = 2,
};

struct file_stats_s {
    int64_t all_total_size;   // 总量
    int64_t all_current_size;   // 当前已接收量
    int64_t cast_time_ms;   // 最大已用时间
};

class ProgressCallInterface : public std::enable_shared_from_this<ProgressCallInterface>
{
public:
    virtual bool onProgress(const std::string &path, uint64_t current, uint64_t total) = 0;

    virtual void onWebChanged(int state, std::string msg) = 0;
};


class WebInterface
{
public:
    void setCallback(std::shared_ptr<ProgressCallInterface> callback)
    {
        _callback = callback;
    }

    void setCallbackA(const std::function<bool(const std::string &path, uint64_t current, uint64_t total)> &cb) {
        callback = cb;
    }
protected:
    std::shared_ptr<ProgressCallInterface> _callback { nullptr };
    std::function<bool(const std::string &path, uint64_t current, uint64_t total)> callback;
};


#endif // SYNCSTATUS_H
