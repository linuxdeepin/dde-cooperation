// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DISCOVERYJOB_H
#define DISCOVERYJOB_H

#include <QObject>

class DiscoveryJob : public QObject
{
    Q_OBJECT
public:
    explicit DiscoveryJob(QObject *parent = nullptr);

signals:

public slots:
};

#endif // DISCOVERYJOB_H