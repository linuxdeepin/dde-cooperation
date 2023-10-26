// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <co/time.h>
#include <co/stl.h>
#include <memory>
#include <functional>

namespace searchlight {

class __coapi Discoverer {

public:
    /*!
    * Represents a discovered service
    * */
    struct service
    {
        fastring service_name; // the name of the service
        fastring endpoint; //sockaddr 地址转换成 "ip:port" 形式的字符串
        fastring info; //json 格式的服务节点信息
        int64_t last_seen; //last see time

        bool operator<(const service& o) const
        {
            // last_seen is ignored
            return std::tie(service_name, endpoint) <
                std::tie(o.service_name, o.endpoint);
        }

        bool operator==(const service& o) const
        {
            // again, last_seen is ignored
            return std::tie(service_name, endpoint) ==
                std::tie(o.service_name, o.endpoint);
        }

        // this uses "name injection"
        friend fastream& operator<<(fastream& os, const Discoverer::service& service)
        {
            os << service.service_name << " on " << "(" << service.endpoint << "): " <<
                service.info;
            return os;
        }
    };

    /// a set of discovered services
    typedef co::set<service> services;

    /// this callback gets called, when ever the set of available services changes
    typedef std::function<void(const services& services)> on_services_changed_t;

    Discoverer(const fastring& listen_for_service, // the service to watch out for
               const on_services_changed_t on_services_changed // callback discovered services changes
              );
    ~Discoverer();

    void start();

    bool started();

    void exit();
private:
    void handle_message(const fastring& message, const fastring& sender_endpoint);
    bool remove_idle_services();

    bool _stop = true;

    co::Timer _timer;
    const fastring _listen_for_service;
    const on_services_changed_t _on_services_changed;

    services _discovered_services;

    DISALLOW_COPY_AND_ASSIGN(Discoverer);
};


class __coapi Announcer {
public:
    Announcer(const fastring& service_name, // the name of the announced service
              const uint16_t service_port, // the port where the service listens on
              const fastring& info // the extend info will be announced
              );
    ~Announcer();

    // update the announce info
    void updateBase(const fastring &info);

    // append app's info
    void appendApp(const fastring &info);

    // remove app's info
    void removeApp(const fastring &info);

    // remove app's info by name
    void removeAppbyName(const fastring &name);

    // start announce
    void start();

    bool started();

    // exit announce
    void exit();

private:
    int sameApp(const fastring &info);

private:
    bool _stop = true;

    const fastring _service_name;
    const uint16_t _service_port;

    fastring _base_info;
    co::vector<fastring> _app_infos;

    DISALLOW_COPY_AND_ASSIGN(Announcer);
};

} // searchlight
