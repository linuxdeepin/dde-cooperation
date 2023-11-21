// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHARECOOPERATIONSERVICE_H
#define SHARECOOPERATIONSERVICE_H

#include <co/fastring.h>
#include "service/comshare.h"

#include <QObject>
#include <QProcess>
#include <QSettings>

class CooConfig;
class ShareCooperationService : public QObject
{
    Q_OBJECT
public:
    ~ShareCooperationService() override;
    static ShareCooperationService *instance();

    void setBarrierType(BarrierType type);
    BarrierType barrierType() const;

    void restartBarrier();

signals:

public slots:
    bool startBarrier();
    void stopBarrier();

protected slots:
    void barrierFinished(int exitCode, QProcess::ExitStatus);
    void appendLogRaw(const QString& text, bool error);
    void logOutput();
    void logError();
private:
    explicit ShareCooperationService(QObject *parent = nullptr);

protected:
    CooConfig& cooConfig() { return *_cooConfig; }
    QProcess* barrierProcess() { return _pBarrier; }
    void setBarrierProcess(QProcess* p) { _pBarrier = p; }

    QString configFilename();
    QString getScreenName();
    QString address();
    QString appPath(const QString& name);

    bool clientArgs(QStringList& args, QString& app);
    bool serverArgs(QStringList& args, QString& app);

private:
    CooConfig* _cooConfig;
    QProcess* _pBarrier;
    BarrierType _brrierType;

    bool _expectedRunning = false;
};

#endif // SHARECOOPERATIONSERVICE_H
