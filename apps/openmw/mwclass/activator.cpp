#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwphysics/physicssystem.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/npcstats.hpp"


namespace MWClass
{

    void Activator::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model, true);
        }
    }

    void Activator::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWPhysics::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string Activator::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    bool Activator::isActivator() const
    {
        return true;
    }

    bool Activator::useAnim() const
    {
        return true;
    }

    std::string Activator::getName (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        return ref->mBase->mName;
    }

    std::string Activator::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return ref->mBase->mScript;
    }

    void Activator::registerSelf()
    {
        std::shared_ptr<Class> instance (new Activator);

        registerClass (typeid (ESM::Activator).name(), instance);
    }

    bool Activator::hasToolTip (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        return (ref->mBase->mName != "");
    }

    bool Activator::allowTelekinesis(const MWWorld::ConstPtr &ptr) const {
        return false;
    }

    MWGui::ToolTipInfo Activator::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(count);

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }

    std::shared_ptr<MWWorld::Action> Activator::activate(const MWWorld::Ptr &ptr, const MWWorld::Ptr &actor) const
    {
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfActivator");

            std::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }
        return std::shared_ptr<MWWorld::Action>(new MWWorld::NullAction);
    }


    MWWorld::Ptr Activator::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }
}
