// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SHARECOOPERATIONSERVICE_H
#define SHARECOOPERATIONSERVICE_H

#include <co/fastring.h>
#include "service/comshare.h"
#include "common/commonstruct.h"

#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QTextStream>

class CooConfig;
class ShareCooperationService : public QObject
{
    Q_OBJECT
public:
    ~ShareCooperationService() override;
    static ShareCooperationService *instance();

    void setBarrierType(BarrierType type);
    BarrierType barrierType() const;

    bool restartBarrier();

    bool setServerConfig(const ShareServerConfig &config);
    bool setClientTargetIp(const QString &ip, const int &port);

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
    QString checkParam(const ShareServerConfig &config);
    void setScreen(const ShareServerConfig &config, QTextStream *stream);
    void setScreenLink(const ShareServerConfig &config, QTextStream *stream);
    void setScreenOptions(const ShareServerConfig &config, QTextStream *stream);

private:
    CooConfig* _cooConfig;
    QProcess* _pBarrier;
    BarrierType _brrierType;
    QString _barrierConfig;

    bool _expectedRunning = false;
};

#endif // SHARECOOPERATIONSERVICE_H
