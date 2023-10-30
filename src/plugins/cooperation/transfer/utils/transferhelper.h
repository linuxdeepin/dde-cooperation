// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TRANSFERHELPER_H
#define TRANSFERHELPER_H

#include "global_defines.h"

#include <QVariantMap>

namespace cooperation_transfer {

class TransferHelperPrivate;
class TransferHelper : public QObject
{
    Q_OBJECT

public:
    static TransferHelper *instance();

    void init();
    void sendFiles(const QString &ip, const QString &devName, const QStringList &fileList);
    TransferStatus transferStatus();

    static void buttonClicked(const QVariantMap &info);
    static bool buttonVisible(const QVariantMap &info);
    static bool buttonClickable(const QVariantMap &info);

public Q_SLOTS:
    void onConnectStatusChanged(int result, const QString &msg);
    void onTransJobStatusChanged(int id, int result, const QString &msg);
    void onFileTransStatusChanged(const QString &status);
    void cancelTransfer();
    void onConfigChanged(const QString &group, const QString &key, const QVariant &value);

private:
    explicit TransferHelper(QObject *parent = nullptr);
    ~TransferHelper();

private:
    QSharedPointer<TransferHelperPrivate> d { nullptr };
};

}   // namespace cooperation_transfer

#endif   // TRANSFERHELPER_H
