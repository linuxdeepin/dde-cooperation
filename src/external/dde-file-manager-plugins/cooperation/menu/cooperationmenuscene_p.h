// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COOPERATIONMENUSCENE_P_H
#define COOPERATIONMENUSCENE_P_H

#include <QMenu>

namespace dfmplugin_cooperation {

class CooperationMenuScene;
class CooperationMenuScenePrivate
{
public:
    explicit CooperationMenuScenePrivate(CooperationMenuScene *qq);

    void createAction(QMenu *menu, const QString &actName, bool isSubAct = false, bool checkable = false);
    void updateMenu(QMenu *menu);

public:
    CooperationMenuScene *q;

    QList<QUrl> selectFiles;
    bool isEmptyArea { false };
    QMap<QString, QAction *> predicateAction;   // id -- instance
    QMap<QString, QString> predicateName;   // id -- text
};

}   // namespace dfmplugin_cooperation

#endif   // COOPERATIONMENUSCENE_P_H
