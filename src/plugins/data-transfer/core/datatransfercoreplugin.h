// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DATATRANSFERCOREPLUGIN_H
#define DATATRANSFERCOREPLUGIN_H

#include <dde-cooperation-framework/dpf.h>

QT_BEGIN_NAMESPACE
class QQmlApplicationEngine;
QT_END_NAMESPACE

namespace data_transfer_core {

class DataTransferCorePlugin : public DPF_NAMESPACE::Plugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.deepin.plugin.datatransfer" FILE "datatransfercoreplugin.json")

    DPF_EVENT_NAMESPACE(deepin_cross_data_transfer)

public:
    virtual void initialize() override;
    virtual bool start() override;

private:
    bool loadMainPage();

private:
    QQmlApplicationEngine *engine { nullptr };
};

}
#endif   // DATATRANSFERCOREPLUGIN_H
