// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationhelper.h"
#include "dialogs/filetransfersettingsdialog.h"

#include <DSettingsOption>
#include <QApplication>
#include <QDebug>
#include <QPushButton>
#include <QLabel>

using namespace dfmplugin_cooperation;

QPair<QWidget *, QWidget *> CooperationHelper::createSettingButton(QObject *opt)
{
    qDebug() << "Creating setting button for cooperation options";
    auto option = qobject_cast<Dtk::Core::DSettingsOption *>(opt);
    auto lab = new QLabel(option->name());
    auto btn = new QPushButton(option->defaultValue().toString());
    btn->setMaximumWidth(190);

    QObject::connect(btn, &QPushButton::clicked, option, [] {
        qDebug() << "Setting button clicked, showing dialog";
        showSettingDialog();
    });

    qInfo() << "Successfully created setting button pair for:" << option->name();
    return qMakePair(lab, btn);
}

void CooperationHelper::showSettingDialog()
{
    qDebug() << "Preparing to show file transfer settings dialog";
    QWidget *parent { nullptr };
    for (auto w : qApp->topLevelWidgets()) {
        auto name = w->objectName();
        if (name == "DSettingsDialog") {
            parent = w;
            break;
        }
    }

    FileTransferSettingsDialog d(parent);
    qInfo() << "Showing file transfer settings dialog";
    d.exec();
}
