// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ReconnectDialog.h"

#include <QLabel>

ReconnectDialog::ReconnectDialog(const QString &machineName)
    : DDialog() {
    setAttribute(Qt::WA_QuitOnClose);
    setFocus(Qt::MouseFocusReason);

    auto *titleLabel = new QLabel(this);
    QFont font = titleLabel->font();
    font.setBold(true);
    font.setPixelSize(16);
    titleLabel->setFont(font);
    titleLabel->setText(tr("PC Collaboration:"));
    addContent(titleLabel, Qt::AlignTop | Qt::AlignHCenter);

    QString content = QString(tr("Failed to connect to %1")).arg(machineName);
    auto *contentLabel = new QLabel(this);
    contentLabel->setText(content);
    addContent(contentLabel, Qt::AlignBottom | Qt::AlignHCenter);

    QStringList btnTexts;
    btnTexts << tr("Cancel") << tr("Try Again");
    addButtons(btnTexts);

    connect(this, &ReconnectDialog::buttonClicked, this, [=](int index, const QString &text) {
        Q_UNUSED(text);
        bool tryAgain = index == 1;
        emit onOperated(tryAgain);
    });

    connect(this, &ReconnectDialog::closed, this, [=]() { emit onOperated(false); });
}