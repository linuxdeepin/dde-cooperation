// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DISCOVERYJOB_H
#define DISCOVERYJOB_H

#include <QObject>
#include <QReadWriteLock>
#include <co/stl.h>

class DiscoveryJob : public QObject
{
    Q_OBJECT
public:
    void discovererRun();
    void announcerRun(const fastring &info);
    void stopDiscoverer();
    void stopAnnouncer();

    void updateAnnouncBase(const fastring info);
    void updateAnnouncApp(bool remove, const fastring info);
    void removeAppbyName(const fastring name);

    co::list<fastring> getNodes();

    static DiscoveryJob *instance()
    {
        static DiscoveryJob ins;
        return &ins;
    }

signals:
    void sigNodeChanged(bool found, QString info);

private:
    void compareOldAndNew(const fastring &uid, const QString &cur,
                          const co::lru_map<fastring, std::pair<fastring, bool>>::iterator &it);

private:
    explicit DiscoveryJob(QObject *parent = nullptr);
    virtual ~DiscoveryJob();
    void *_discoverer_p;
    void *_announcer_p;

    //<uuid, <peerinfo, exist>>
    QReadWriteLock _dis_lock;
    co::lru_map<fastring, std::pair<fastring, bool>> _dis_node_maps;
};

#endif // DISCOVERYJOB_H
