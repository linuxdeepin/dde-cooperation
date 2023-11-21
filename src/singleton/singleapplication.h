// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#ifdef WIN32
#include <QApplication>
typedef QApplication CrossApplication;
#else
#include <DApplication>
typedef DTK_WIDGET_NAMESPACE::DApplication CrossApplication;
#endif

QT_BEGIN_NAMESPACE
class QLocalServer;
QT_END_NAMESPACE

namespace deepin_cross {

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<SingleApplication *>(CrossApplication::instance()))

class SingleApplication : public CrossApplication
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
