// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication>

QT_BEGIN_NAMESPACE
class QLocalServer;
QT_END_NAMESPACE

namespace deepin_cross {
class SingleApplication : public QApplication
{
    Q_OBJECT
public:
    explicit SingleApplication(int &argc, char **argv, int = ApplicationFlags);
    ~SingleApplication();
    bool setSingleInstance(const QString &key);
    void closeServer();

Q_SIGNALS:
    void raiseWindow();

protected:
    void initConnect();

protected Q_SLOTS:
    void handleNewConnection();
    QString userServerName(const QString &key);

private:
    QLocalServer *localServer { nullptr };
};
}

#endif   // SINGLEAPPLICATION_H
