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
    _dis_node_maps.clear();
}

DiscoveryJob::~DiscoveryJob()
{
    _dis_node_maps.clear();

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
            // loop and set may not exist
            for (auto it = _dis_node_maps.begin(); it != _dis_node_maps.end(); ++it) {
                it->second.second = false;
            }

            for(auto& service : services) {
                //DLOG << "discovered: " << service;
                co::Json node;
                node.parse_from(service.info);
                fastring uid = node.get("uuid").as_string();

                auto it = _dis_node_maps.find(uid);
                if (it != _dis_node_maps.end()) {
                    // has been recorded, markd it exist
                    it->second.second = true;
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

void DiscoveryJob::updateAnnouncNote(const fastring info)
{
    ((searchlight::Announcer*)_announcer_p)->update(info);
}

co::list<fastring> DiscoveryJob::getNodes()
{
    co::list<fastring> notes;
    for (auto it = _dis_node_maps.begin(); it != _dis_node_maps.end(); ++it) {
        notes.push_back(it->second.first);
    }
    return notes;
}
