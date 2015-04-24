
#include "activator.hpp"

#include <components/esm/loadacti.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/npcstats.hpp"


namespace MWClass
{
    std::string Activator::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM::Activator>()->mBase->mId;
    }

    void Activator::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty()) {
            MWRender::Actors& actors = renderingInterface.getActors();
            actors.insertActivator(ptr, model);
        }
    }

    void Activator::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Activator::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Activator::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return ref->mBase->mName;
    }

    std::string Activator::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return ref->mBase->mScript;
    }

    void Activator::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Activator);

        registerClass (typeid (ESM::Activator).name(), instance);
    }

    bool Activator::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Activator::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> Activator::activate(const MWWorld::Ptr &ptr, const MWWorld::Ptr &actor) const
    {
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfActivator");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::NullAction);
    }


    MWWorld::Ptr
    Activator::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Activator> *ref =
            ptr.get<ESM::Activator>();

        return MWWorld::Ptr(&cell.get<ESM::Activator>().insert(*ref), &cell);
    }
}
