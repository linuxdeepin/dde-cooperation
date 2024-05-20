// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../maincontroller.h"
#include "utils/cooperationutil.h"
#include "configs/settings/configmanager.h"
#include "common/constant.h"
#include "common/commonutils.h"

#include <QStandardPaths>
#include <QJsonDocument>
#include <QDir>

#include <mutex>

using namespace cooperation_core;

void MainController::initNotifyConnect()
{
    transTimer.setInterval(10 * 1000);
    transTimer.setSingleShot(true);

    connect(&transTimer, &QTimer::timeout, this, &MainController::onConfirmTimeout);
    
    if (!cooperationDlg) {
        cooperationDlg = new CooperationTransDialog(qApp->activeWindow());
        connect(cooperationDlg, &CooperationTransDialog::accepted, this, &MainController::onAccepted);
        connect(cooperationDlg, &CooperationTransDialog::rejected, this, &MainController::onRejected);
        connect(cooperationDlg, &CooperationTransDialog::canceled, this, &MainController::onCanceled);
        connect(cooperationDlg, &CooperationTransDialog::completed, this, &MainController::onCompleted);
        connect(cooperationDlg, &CooperationTransDialog::viewed, this, &MainController::onViewed);
    }
}

void MainController::registApp()
{
    auto value = ConfigManager::instance()->appAttribute(AppSettings::GenericGroup, AppSettings::StoragePathKey);
    auto storagePath = value.isValid() ? value.toString() : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    static std::once_flag flag;
    std::call_once(flag, [&storagePath] { CooperationUtil::instance()->setStorageConfig(storagePath); });

    auto info = CooperationUtil::deviceInfo();
    info.insert(AppSettings::CooperationEnabled, true);

    auto doc = QJsonDocument::fromVariant(info);
    CooperationUtil::instance()->registAppInfo(doc.toJson());
}

void MainController::updateProgress(int value, const QString &msg)
{
    static QString title(tr("Receiving files from \"%1\""));
    cooperationDlg->showProgressDialog(
        title.arg(deepin_cross::CommonUitls::elidedText(requestFrom, Qt::ElideMiddle, MID_FRONT)));
    cooperationDlg->updateProgressData(value, msg);
}

void MainController::waitForConfirm(const QString &name)
{
    isReplied = false;
    recvFilesSavePath.clear();
    isRequestTimeout = false;
    requestFrom = name;
    transTimer.start();

    cooperationDlg->showConfirmDialog(deepin_cross::CommonUitls::elidedText(name, Qt::ElideMiddle, 15));
    cooperationDlg->show();
}

void MainController::showToast(bool result, const QString &msg)
{
    cooperationDlg->showResultDialog(result, msg);
}

void MainController::onNetworkMiss()
{
    static QString msg(tr("Network not connected, file delivery failed this time.\
                             Please connect to the network and try again!"));
    showToast(false, msg);
}
