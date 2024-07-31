﻿// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LISTENER_H
#define LISTENER_H

#include <dde-cooperation-framework/dde_cooperation_framework_global.h>

#include <QObject>

DPF_BEGIN_NAMESPACE

class DPF_EXPORT Listener final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Listener)

public:
    explicit Listener(QObject *parent = nullptr);
    static Listener *instance();

Q_SIGNALS:
    void pluginInitialized(const QString &iid, const QString &name);
    void pluginStarted(const QString &iid, const QString &name);
    void pluginsInitialized();
    void pluginsStarted();
};

DPF_END_NAMESPACE
#define dpfListener ::DPF_NAMESPACE::Listener::instance()

#endif   // LISTENER_H
