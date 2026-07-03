// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONNECTWIDGET_H
#define CONNECTWIDGET_H
#ifdef __linux__
#    include <QFrame>

class QLabel;
class QLineEdit;
class QHBoxLayout;
class QPushButton;
class QTimer;
class ConnectWidget : public QFrame
{
    Q_OBJECT

public:
    ConnectWidget(QWidget *parent = nullptr);
    ~ConnectWidget();

    void initConnectLayout();

public slots:
    void nextPage();
    void backPage();
    void themeChanged(int theme);

private:
    void initUI();
    void onPortEditingFinished();
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *ipLabel = nullptr;
    QLabel *ipLabel1 = nullptr;
    QLabel *portLabel = nullptr;
    QLabel *WarnningLabel = nullptr;
    QHBoxLayout *connectLayout = nullptr;
    int remainingTime = 300;
    QPushButton *backButton = nullptr;
    QLabel *separatorLabel = nullptr;

    // 端口输入（在 IP 框内）
    QLineEdit *portInput{ nullptr };
    QTimer *portDebounceTimer{ nullptr };
    int m_savedPort{ 0 };
};
#endif
#endif
