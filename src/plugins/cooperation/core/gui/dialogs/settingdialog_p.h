// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SETTINGDIALOG_P_H
#define SETTINGDIALOG_P_H

#include "global_defines.h"
#include "gui/widgets/filechooseredit.h"
#include "gui/widgets/settingitem.h"

#include <QObject>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QScrollArea>

namespace cooperation_core {

class SettingDialog;
class SettingDialogPrivate : public QObject
{
    Q_OBJECT
    friend class SettingDialog;

public:
    explicit SettingDialogPrivate(SettingDialog *qq);
    ~SettingDialogPrivate();

    void initWindow();
    void initTitleBar();

public Q_SLOTS:
    void onFindComboBoxValueChanged(int index);
    void onConnectComboBoxValueChanged(int index);
    void onTransferComboBoxValueChanged(int index);
    void onNameEditingFinished();
    void onDeviceShareButtonClicked(bool clicked);
    void onClipboardShareButtonClicked(bool clicked);
    void onFileChoosed(const QString &path);

private:
    void createBasicWidget();
    void createDeviceShareWidget();
    void createTransferWidget();
    void createClipboardShareWidget();

private:
    SettingDialog *q { nullptr };
    QVBoxLayout *mainLayout { nullptr };
    QWidget *mainWidget { nullptr };
    QVBoxLayout *contentLayout { nullptr };

    QComboBox *findCB { nullptr };
    QComboBox *connectCB { nullptr };
    QComboBox *transferCB { nullptr };
    QLineEdit *nameEdit { nullptr };
    CooperationSwitchButton *devShareSwitchBtn { nullptr };
    CooperationSwitchButton *clipShareSwitchBtn { nullptr };
    FileChooserEdit *chooserEdit { nullptr };
    SettingItem *connectItem { nullptr };

    QStringList findComboBoxInfo;
    QList<QPair<QString, QString>> connectComboBoxInfo;
    QStringList transferComboBoxInfo;
};

}   // namespace cooperation_core

#endif   // SETTINGDIALOG_P_H
