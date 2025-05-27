// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cooperationmenuscene.h"
#include "cooperationmenuscene_p.h"

#include <dfm-base/dfm_menu_defines.h>

#include <QUrl>
#include <QProcess>

inline constexpr char kFileTransfer[] { "file-transfer" };

using namespace dfmplugin_cooperation;
DFMBASE_USE_NAMESPACE

AbstractMenuScene *CooperationMenuCreator::create()
{
    qDebug() << "Creating cooperation menu scene";
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
    qDebug() << "Cooperation menu scene created";
    d->predicateName[kFileTransfer] = tr("File transfer");
    qDebug() << "Initialized menu item text for file transfer";
}

CooperationMenuScene::~CooperationMenuScene()
{
}

QString CooperationMenuScene::name() const
{
    qDebug() << "Getting cooperation menu scene name";
    return CooperationMenuCreator::name();
}

bool CooperationMenuScene::initialize(const QVariantHash &params)
{
    qDebug() << "Initializing cooperation menu scene";
    
    d->selectFiles = params.value(MenuParamKey::kSelectFiles).value<QList<QUrl>>();
    d->isEmptyArea = params.value(MenuParamKey::kIsEmptyArea).toBool();
    qDebug() << "Got" << d->selectFiles.size() << "selected files, isEmptyArea:" << d->isEmptyArea;

    if (d->selectFiles.isEmpty() || !d->selectFiles.first().isLocalFile()) {
        qWarning() << "No valid local files selected for cooperation menu";
        return false;
    }

    auto subScenes = subscene();
    setSubscene(subScenes);
    qDebug() << "Subscenes initialized for cooperation menu";

    bool result = AbstractMenuScene::initialize(params);
    qDebug() << "Cooperation menu scene initialized successfully:" << result;
    return result;
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
    qDebug() << "Creating cooperation menu items";
    
    if (!parent) {
        qWarning() << "Cannot create menu items - null parent menu";
        return false;
    }

    if (!d->isEmptyArea) {
        qDebug() << "Adding file transfer menu item";
        auto transAct = parent->addAction(d->predicateName.value(kFileTransfer));
        d->predicateAction[kFileTransfer] = transAct;
        transAct->setProperty(ActionPropertyKey::kActionID, kFileTransfer);
        qInfo() << "Added file transfer menu item";
    }

    bool result = AbstractMenuScene::create(parent);
    qDebug() << "Cooperation menu items created successfully:" << result;
    return result;
}

void CooperationMenuScene::updateState(QMenu *parent)
{
    qDebug() << "Updating cooperation menu state";
    
    if (!d->isEmptyArea) {
        qDebug() << "Updating file transfer menu item position";
        auto actions = parent->actions();
        parent->removeAction(d->predicateAction[kFileTransfer]);

        for (auto act : actions) {
            if (act->isSeparator())
                continue;

            auto actId = act->property(ActionPropertyKey::kActionID).toString();
            if (actId == "send-to") {
                auto subMenu = act->menu();
                if (subMenu) {
                    auto subActs = subMenu->actions();
                    subActs.insert(0, d->predicateAction[kFileTransfer]);
                    subMenu->addActions(subActs);
                    act->setVisible(true);
                    qDebug() << "Moved file transfer menu item to 'Send To' submenu";
                    break;
                }
            }
        }
    }

    AbstractMenuScene::updateState(parent);
    qDebug() << "Cooperation menu state updated";
}

bool CooperationMenuScene::triggered(QAction *action)
{
    qDebug() << "Handling cooperation menu action trigger";
    
    auto actionId = action->property(ActionPropertyKey::kActionID).toString();
    if (!d->predicateAction.contains(actionId)) {
        qDebug() << "Action not handled by cooperation menu, delegating to parent";
        return AbstractMenuScene::triggered(action);
    }

    if (actionId == kFileTransfer) {
        qInfo() << "File transfer menu item triggered";
        QStringList fileList;
        for (auto &url : d->selectFiles)
            fileList << url.toLocalFile();

        QStringList arguments;
        arguments << "-s"
                  << fileList;

        return QProcess::startDetached("dde-cooperation-transfer", arguments);
    }

    return true;
}
