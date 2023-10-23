// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEVICEITEM_H
#define DEVICEITEM_H

#include "global_defines.h"
#ifdef WIN32
#else
#    include "gui/linux/backgroundwidget.h"
#endif

#include <QLabel>
#include <QIcon>

namespace cooperation_workspace {

class ButtonBoxWidget;
class StateLabel : public QLabel
{
    Q_OBJECT
public:
    explicit StateLabel(QWidget *parent = nullptr);

    void setState(ConnectState state) { st = state; }
    ConnectState state() const { return st; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    ConnectState st;
};

class DeviceItem : public BackgroundWidget
{
    Q_OBJECT
public:
    using ButtonStateCallback = std::function<bool(const QVariantMap &)>;
    using ClickedCallback = std::function<void(const QVariantMap &)>;
    struct Operation
    {
        QString id;
        QString description;
        QString icon;
        int location;
        int style;
        ButtonStateCallback visibleCb;
        ButtonStateCallback clickableCb;
        ClickedCallback clickedCb;
    };

    explicit DeviceItem(QWidget *parent = nullptr);

    void setDeviceName(const QString &name);
    QString deviceName() const;

    void setIPText(const QString &ipStr);
    QString ipText() const;

    void setDeviceState(ConnectState state);
    ConnectState deviceState() const;

    void setOperations(const QList<Operation> &operations);
    void updateOperations();

public Q_SLOTS:
    void onButtonClicked(int index);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void initUI();
    void initConnect();
    void setLabelFont(QLabel *label, int pointSize, int weight);

private:
    QLabel *iconLabel { nullptr };
    QLabel *nameLabel { nullptr };
    QLabel *ipLabel { nullptr };
    StateLabel *stateLabel { nullptr };
    ButtonBoxWidget *btnBoxWidget { nullptr };

    QMap<int, Operation> indexOperaMap;
};

}   // namespace cooperation_workspace

Q_DECLARE_METATYPE(cooperation_workspace::DeviceItem::ButtonStateCallback)
Q_DECLARE_METATYPE(cooperation_workspace::DeviceItem::ClickedCallback)

#endif   // DEVICEITEM_H
