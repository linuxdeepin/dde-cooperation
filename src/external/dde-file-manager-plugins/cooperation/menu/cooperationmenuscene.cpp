// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationmenuscene.h"
#include "cooperationmenuscene_p.h"

#include <dfm-base/dfm_menu_defines.h>

#include <QUrl>

inline constexpr char kFileTransfer[] { "file-transfer" };

using namespace dfmplugin_cooperation;
DFMBASE_USE_NAMESPACE

AbstractMenuScene *CooperationMenuCreator::create()
{
    return new CooperationMenuScene();
}

CooperationMenuScenePrivate::CooperationMenuScenePrivate(CooperationMenuScene *qq)
    : q(qq)
{
}

CooperationMenuScene::CooperationMenuScene(QObject *parent)
    : AbstractMenuScene(parent),
      d(new CooperationMenuScenePrivate(this))
{
    d->predicateName[kFileTransfer] = tr("File transfer");
}

CooperationMenuScene::~CooperationMenuScene()
{
}

QString CooperationMenuScene::name() const
{
    return CooperationMenuCreator::name();
}

bool CooperationMenuScene::initialize(const QVariantHash &params)
{
    d->selectFiles = params.value(MenuParamKey::kSelectFiles).value<QList<QUrl>>();
    d->isEmptyArea = params.value(MenuParamKey::kIsEmptyArea).toBool();

    auto subScenes = subscene();
    setSubscene(subScenes);

    return AbstractMenuScene::initialize(params);
}

AbstractMenuScene *CooperationMenuScene::scene(QAction *action) const
{
    if (action == nullptr)
        return nullptr;

    if (!d->predicateAction.key(action).isEmpty())
        return const_cast<CooperationMenuScene *>(this);

    return AbstractMenuScene::scene(action);
}

bool CooperationMenuScene::create(QMenu *parent)
{
    if (!parent)
        return false;

    if (!d->isEmptyArea) {
        auto act = parent->addAction(d->predicateName.value(kFileTransfer));
        d->predicateAction[kFileTransfer] = act;
        act->setProperty(ActionPropertyKey::kActionID, kFileTransfer);
    }

    return AbstractMenuScene::create(parent);
}

void CooperationMenuScene::updateState(QMenu *parent)
{
    AbstractMenuScene::updateState(parent);
}

bool CooperationMenuScene::triggered(QAction *action)
{
    auto actionId = action->property(ActionPropertyKey::kActionID).toString();
    if (!d->predicateAction.contains(actionId))
        return AbstractMenuScene::triggered(action);

    if (actionId == kFileTransfer) {
        // TODO
    }

    return true;
}
