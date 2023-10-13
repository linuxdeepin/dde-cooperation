// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationhelper.h"
#include "dialogs/filetransfersettingsdialog.h"

#include <DSettingsOption>

#include <QPushButton>
#include <QLabel>

using namespace dfmplugin_cooperation;

QPair<QWidget *, QWidget *> CooperationHelper::createSettingButton(QObject *opt)
{
    auto option = qobject_cast<Dtk::Core::DSettingsOption *>(opt);
    auto lab = new QLabel(option->name());
    auto btn = new QPushButton(option->defaultValue().toString());
    btn->setMaximumWidth(190);

    QObject::connect(btn, &QPushButton::clicked, option, [] {
        showSettingDialog();
    });

    return qMakePair(lab, btn);
}

void CooperationHelper::showSettingDialog()
{
    FileTransferSettingsDialog d;
    d.exec();
}
