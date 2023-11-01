// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "discoveryjob.h"
#include "searchlight.h"

#include "common/constant.h"
#include "co/log.h"
#include "co/json.h"

DiscoveryJob::DiscoveryJob(QObject *parent)
    : QObject(parent)
{
    {
        QWriteLocker lk(&_dis_lock);
        _dis_node_maps.clear();
    }
}

DiscoveryJob::~DiscoveryJob()
{
    {
        QWriteLocker lk(&_dis_lock);
        _dis_node_maps.clear();
    }
    // free discoverer pointer
    if (_discoverer_p) {
        auto p = (searchlight::Discoverer*)_discoverer_p;
        if (!p->started()) co::del(p);
        _discoverer_p = nullptr;
    }

    // free announcer pointer
    if (_announcer_p) {
        auto p = (searchlight::Announcer*)_announcer_p;
        if (!p->started()) co::del(p);
        _announcer_p = nullptr;
    }
}

void DiscoveryJob::discovererRun()
{
    _discoverer_p = co::make<searchlight::Discoverer>(
        "ulink_service",
        [this](const searchlight::Discoverer::services& services)
        {
            QWriteLocker lk(&_dis_lock);
            // loop and set may not exist
            for (auto it = _dis_node_maps.begin(); it != _dis_node_maps.end(); ++it) {
                it->second.second = false;
            }

            for(auto& service : services) {
                //DLOG << "discovered: " << service;

                co::Json node;
                node.parse_from(service.info);
                co::Json osjson = node.get("os");
                if (osjson.is_null() || !osjson.has_member("uuid")) {
                    //DLOG << "found error format: " << service;
                    continue;
                }

                fastring uid = osjson.get("uuid").as_string();

                auto it = _dis_node_maps.find(uid);
                if (it != _dis_node_maps.end()) {
                    // has been recorded, markd it exist
                    it->second.second = true;
                    if (it->second.first.compare(service.info) != 0) {
                        co::Json oldnode;
                        oldnode.parse_from(it->second.first);
                        co::Json oldapps = oldnode.get("apps");
                        co::Json curapps = node.get("apps");

                        if (!oldapps.is_null() && curapps.is_null()) {
                            //node has been unregister or losted.
                            emit sigNodeChanged(false, QString(it->second.first.c_str()));
                            _dis_node_maps.erase(it);
                        } else {
                            //node info has been updated, force update now.
                            _dis_node_maps.erase(it);
                            emit sigNodeChanged(true, QString(service.info.c_str()));
                            _dis_node_maps.insert(uid, std::make_pair(service.info, true));
                        }
                    }
                } else {
                    //new node discovery.
                    //DLOG << "new peer found: " << node.str();
                    emit sigNodeChanged(true, QString(service.info.c_str()));
                    _dis_node_maps.insert(uid, std::make_pair(service.info, true));
                }
            }

            // loop and notify all not exist node.
            for (auto it = _dis_node_maps.begin(); it != _dis_node_maps.end(); ++it) {
                if (!it->second.second) {
                    //DLOG << "peer losted: " << it->second.first;
                    emit sigNodeChanged(false, QString(it->second.first.c_str()));
                    _dis_node_maps.erase(it);
                }
            }
        }
    );
    ((searchlight::Discoverer*)_discoverer_p)->start();
}

void DiscoveryJob::announcerRun(const fastring &info)
{
    _announcer_p = co::make<searchlight::Announcer>("ulink_service", UNI_RPC_PORT_BASE, info);

    ((searchlight::Announcer*)_announcer_p)->start();
}

void DiscoveryJob::stopDiscoverer()
{
    ((searchlight::Discoverer*)_discoverer_p)->exit();
}

void DiscoveryJob::stopAnnouncer()
{
    ((searchlight::Announcer*)_announcer_p)->exit();
}

void DiscoveryJob::updateAnnouncBase(const fastring info)
{
    ((searchlight::Announcer*)_announcer_p)->updateBase(info);
}

void DiscoveryJob::updateAnnouncApp(bool remove, const fastring info)
{
    if (remove) {
        ((searchlight::Announcer*)_announcer_p)->removeApp(info);
    } else {
        ((searchlight::Announcer*)_announcer_p)->appendApp(info);
    }
}

void DiscoveryJob::removeAppbyName(const fastring name)
{
    ((searchlight::Announcer*)_announcer_p)->removeAppbyName(name);
}

co::list<fastring> DiscoveryJob::getNodes()
{
    co::list<fastring> notes;
    QReadLocker lk(&_dis_lock);
    for (auto it = _dis_node_maps.begin(); it != _dis_node_maps.end(); ++it) {
        notes.push_back(it->second.first);
    }
    return notes;
}
