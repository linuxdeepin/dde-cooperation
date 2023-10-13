// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERPLUGIN_H
#define TRANSFERPLUGIN_H

#include <dde-cooperation-framework/dpf.h>

namespace cooperation_transfer {

class TransferPlugin : public DPF_NAMESPACE::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.cooperation" FILE "transferplugin.json")

    DPF_EVENT_NAMESPACE(cooperation_transfer)

public:
    virtual void initialize() override;
    virtual bool start() override;
    virtual void stop() override;
};

}   // namespace cooperation_transfer

#endif   // TRANSFERPLUGIN_H
