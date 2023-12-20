// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILECHOOSEREDIT_H
#define FILECHOOSEREDIT_H

#include "global_defines.h"

#include <QWidget>
#include <QLabel>
#include <QPushButton>

namespace cooperation_core {

class FileChooserEdit : public QWidget
{
    Q_OBJECT
public:
    explicit FileChooserEdit(QWidget *parent = nullptr);

    void setText(const QString &text);

Q_SIGNALS:
    void fileChoosed(const QString &fileName);

private Q_SLOTS:
    void onButtonClicked();
    void updateSizeMode();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();

    QLabel *pathLabel { nullptr };
    CooperationSuggestButton *fileChooserBtn { nullptr };
};

}   // namespace cooperation_core

#endif   // FILECHOOSEREDIT_H
