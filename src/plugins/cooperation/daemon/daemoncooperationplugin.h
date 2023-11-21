// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DAEMONCOOPERATIONPLUGIN_H
#define DAEMONCOOPERATIONPLUGIN_H

#include <dde-cooperation-framework/dpf.h>

namespace daemon_cooperation {

class DaemonCooperationPlugin : public DPF_NAMESPACE::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.daemon" FILE "daemoncooperationplugin.json")

    DPF_EVENT_NAMESPACE(daemon_cooperation)

public:
    virtual void initialize() override;
    virtual bool start() override;
    virtual void stop() override;

private Q_SLOTS:
    void loadTranslator();
    void onAllPluginsStarted();
};

}

#endif // DAEMONCOOPERATIONPLUGIN_H
