// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONCOREPLUGIN_H
#define COOPERATIONCOREPLUGIN_H

#include <dde-cooperation-framework/dpf.h>

namespace cooperation_core {

class MainWindow;
class CooperaionCorePlugin : public DPF_NAMESPACE::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.cooperation" FILE "cooperationcoreplugin.json")

    DPF_EVENT_NAMESPACE(cooperation_core)

    // slot events
    DPF_EVENT_REG_SLOT(slot_Register_Operation)

public:
    virtual void initialize() override;
    virtual bool start() override;
    virtual void stop() override;

private:
    void bindEvents();
};

}   // namespace cooperation_core

#endif   // COOPERATIONCOREPLUGIN_H
