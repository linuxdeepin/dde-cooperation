// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONSEARCHEDIT_H
#define COOPERATIONSEARCHEDIT_H

#include <QFrame>

class QLineEdit;
class QLabel;
class QToolButton;
namespace cooperation_core {

class CooperationSearchEdit : public QFrame
{
    Q_OBJECT

public:
    explicit CooperationSearchEdit(QWidget *parent = nullptr);

    QString text() const;

Q_SIGNALS:
    void textChanged(const QString &);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private:
    QLineEdit *searchEdit{ nullptr };
    QLabel *searchIcon{ nullptr };
    QToolButton *closeBtn{ nullptr };
};

} // namespace cooperation_core

#endif // COOPERATIONSEARCHEDIT_H
