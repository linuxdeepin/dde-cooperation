// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONCOREPLUGIN_H
#define COOPERATIONCOREPLUGIN_H

#include <QObject>

namespace cooperation_core {

class MainWindow;
class CooperaionCorePlugin : public QObject
{
    Q_OBJECT

public:
    explicit CooperaionCorePlugin(QObject *parent = nullptr);
    ~CooperaionCorePlugin();

    bool start();
    void stop();

private:
    void initialize();
};

}   // namespace cooperation_core

#endif   // COOPERATIONCOREPLUGIN_H
