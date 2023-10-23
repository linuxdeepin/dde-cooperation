// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WORKSPACE_P_H
#define WORKSPACE_P_H

#include "global_defines.h"
#include "workspacewidget.h"
#include "utils/sortfilterworker.h"

#ifdef WIN32
#    include <QMainWindow>
// TODO:
#else
#include <DSearchEdit>
typedef DTK_WIDGET_NAMESPACE::DSearchEdit CooperationSearchEdit;
#endif

#include <QStackedLayout>
#include <QThread>

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
    ~WorkspaceWidgetPrivate();

    void initUI();
    void initConnect();

public Q_SLOTS:
    void onSearchValueChanged(const QString &text);
    void onSortFilterResult(int index, const DeviceInfo &info);
    void onFilterFinished();

Q_SIGNALS:
    void deviceAdded(const DeviceInfo &info);
    void filterDevice(const QString &str);
    void clearDevice();

public:
    WorkspaceWidget *q { nullptr };
    QStackedLayout *stackedLayout { nullptr };
    CooperationSearchEdit *searchEdit { nullptr };
    LookingForDeviceWidget *lfdWidget { nullptr };
    NoNetworkWidget *nnWidget { nullptr };
    NoResultWidget *nrWidget { nullptr };
    DeviceListWidget *dlWidget { nullptr };

    WorkspaceWidget::PageName currentPage;
    QSharedPointer<SortFilterWorker> sortFilterWorker { nullptr };
    QSharedPointer<QThread> workThread { nullptr };
};

}   // namespace cooperation_workspace {

#endif   // WORKSPACE_P_H
