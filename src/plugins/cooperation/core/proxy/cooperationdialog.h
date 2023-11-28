// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONDIALOG_H
#define COOPERATIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QStackedLayout>

namespace cooperation_core {

class ConfirmWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConfirmWidget(QWidget *parent = nullptr);

    void setDeviceName(const QString &name);

Q_SIGNALS:
    void accepted();
    void rejected();

private:
    void init();

    QLabel *msgLabel { nullptr };
    QPushButton *acceptBtn { nullptr };
    QPushButton *rejectBtn { nullptr };
};

class ProgressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProgressWidget(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setProgress(int value, const QString &msg);

Q_SIGNALS:
    void canceled();

private:
    void init();

    QLabel *titleLabel { nullptr };
    QLabel *msgLabel { nullptr };
    QProgressBar *progressBar { nullptr };
    QPushButton *cancelBtn { nullptr };
};

class ResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ResultWidget(QWidget *parent = nullptr);

    void setResult(bool success, const QString &msg);

Q_SIGNALS:
    void completed();
    void viewed();

private:
    void init();

    QLabel *iconLabel { nullptr };
    QLabel *msgLabel { nullptr };
    QPushButton *okBtn { nullptr };
    QPushButton *viewBtn { nullptr };
};

class CooperationTransDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CooperationTransDialog(QWidget *parent = nullptr);

    void showConfirmDialog(const QString &name);
    void showResultDialog(bool success, const QString &msg);
    void showProgressDialog(const QString &title);
    void updateProgressData(int value, const QString &msg);

protected:
    void closeEvent(QCloseEvent *e) override;

Q_SIGNALS:
    void canceled();
    void completed();
    void viewed();

private:
    void init();

    QStackedLayout *mainLayout { nullptr };
    ConfirmWidget *confirmWidget { nullptr };
    ProgressWidget *progressWidget { nullptr };
    ResultWidget *resultWidget { nullptr };
};

}   // namespace cooperation_core

#endif   // COOPERATIONDIALOG_H
