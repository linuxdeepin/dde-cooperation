// SPDX-FileCopyrightText: 2021 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LIFECYCLE_H
#define LIFECYCLE_H

#include <dde-cooperation-framework/lifecycle/pluginmetaobject.h>
#include <dde-cooperation-framework/lifecycle/plugin.h>
#include <dde-cooperation-framework/lifecycle/plugincreator.h>
#include <dde-cooperation-framework/dde_cooperation_framework_global.h>

#include <QString>
#include <QObject>

DPF_BEGIN_NAMESPACE

namespace LifeCycle {
void initialize(const QStringList &IIDs, const QStringList &paths);
void initialize(const QStringList &IIDs, const QStringList &paths, const QStringList &blackNames);
void initialize(const QStringList &IIDs, const QStringList &paths, const QStringList &blackNames,
                const QStringList &lazyNames);

bool isAllPluginsInitialized();
bool isAllPluginsStarted();
QStringList pluginIIDs();
QStringList pluginPaths();
QStringList blackList();
QStringList lazyLoadList();
PluginMetaObjectPointer pluginMetaObj(const QString &pluginName,
                                      const QString version = "");

bool readPlugins();
bool loadPlugins();
void shutdownPlugins();

bool loadPlugin(PluginMetaObjectPointer &pointer);
void shutdownPlugin(PluginMetaObjectPointer &pointer);
}   // namepsace LifeCycle

DPF_END_NAMESPACE

#endif   // LIFECYCLE_H
