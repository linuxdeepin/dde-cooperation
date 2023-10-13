// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WORKSPACE_P_H
#define WORKSPACE_P_H

#include "global_defines.h"

#include <QStackedLayout>
#ifdef WIN32
#include <QMainWindow>
// TODO:
#else
#include <DSearchEdit>
typedef DTK_WIDGET_NAMESPACE::DSearchEdit CooperationSearchEdit;
#endif

namespace cooperation_workspace {

class WorkspaceWidget;
class LookingForDeviceWidget;
class NoNetworkWidget;
class NoResultWidget;
class DeviceListWidget;
class WorkspaceWidgetPrivate : public QObject
{
    Q_OBJECT
public:
    explicit WorkspaceWidgetPrivate(WorkspaceWidget *qq);

    void initUI();

public:
    WorkspaceWidget *q { nullptr };
    QStackedLayout *stackedLayout { nullptr };
    CooperationSearchEdit *searchEdit { nullptr };
    LookingForDeviceWidget *lfdWidget { nullptr };
    NoNetworkWidget *nnWidget { nullptr };
    NoResultWidget *nrWidget { nullptr };
    DeviceListWidget *dlWidget { nullptr };
};

}   // namespace cooperation_workspace {

#endif   // WORKSPACE_P_H
