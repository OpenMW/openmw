#include "activator.hpp"

#include <components/esm/loadacti.hpp>
#include <components/misc/rng.hpp>

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

    std::string Activator::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        std::string model = getModel(ptr);
        if (model.empty())
            return std::string();

        const MWWorld::Store<ESM::Creature> &creaturestore = MWBase::Environment::get().getWorld()->getStore().get<ESM::Creature>();
        std::string creatureId;
        
        for (const ESM::Creature &iter : creaturestore)
        {
            if (iter.mModel.empty())
                continue;

            if (Misc::StringUtils::ciEqual(model, "meshes\\" + iter.mModel))
            {
                creatureId = !iter.mOriginal.empty() ? iter.mOriginal : iter.mId;
                break;
            }
        }

        if (creatureId.empty())
            return std::string();

        const MWWorld::Store<ESM::SoundGenerator> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::SoundGenerator>();

        int type = getSndGenTypeFromName(name);
        std::vector<const ESM::SoundGenerator*> sounds;

        MWWorld::Store<ESM::SoundGenerator>::iterator sound = store.begin();

        while (sound != store.end())
        {
            if (type == sound->mType && !sound->mCreature.empty() && (Misc::StringUtils::ciEqual(creatureId, sound->mCreature)))
                sounds.push_back(&*sound);
            ++sound;
        }

        if (!sounds.empty())
            return sounds[Misc::Rng::rollDice(sounds.size())]->mSound;

        if (type == ESM::SoundGenerator::Land)
            return "Body Fall Large";

        return std::string();
    }

    int Activator::getSndGenTypeFromName(const std::string &name) const
    {
        if (name == "left")
            return 0;
        if (name == "right")
            return 1;
        if (name == "swimleft")
            return 2;
        if (name == "swimright")
            return 3;
        if (name == "moan")
            return 4;
        if (name == "roar")
            return 5;
        if (name == "scream")
            return 6;
        if (name == "land")
            return 7;

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }
}
