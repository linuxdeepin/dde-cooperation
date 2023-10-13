// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WORKSPACEPLUGIN_H
#define WORKSPACEPLUGIN_H

#include <dde-cooperation-framework/dpf.h>

namespace cooperation_workspace {

class WorkspacePlugin : public DPF_NAMESPACE::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.cooperation" FILE "workspaceplugin.json")

    DPF_EVENT_NAMESPACE(cooperation_workspace)

    // slot events
    DPF_EVENT_REG_SLOT(slot_Register_Operation)

public:
    virtual void initialize() override;
    virtual bool start() override;
    virtual void stop() override;

private:
    void bindEvents();
};

} // namespace cooperation_workspace

#endif // WORKSPACEPLUGIN_H
