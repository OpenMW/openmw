#include "activator.hpp"

#include <MyGUI_TextIterator.h>

#include <components/esm3/loadacti.hpp>
#include <components/misc/rng.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
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
#include "../mwrender/vismask.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/npcstats.hpp"


namespace MWClass
{
    Activator::Activator()
        : MWWorld::RegisteredClass<Activator>(ESM::Activator::sRecordId)
    {
    }

    void Activator::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        if (!model.empty())
        {
            renderingInterface.getObjects().insertModel(ptr, model, true);
            ptr.getRefData().getBaseNode()->setNodeMask(MWRender::Mask_Static);
        }
    }

    void Activator::insertObject(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation, MWPhysics::PhysicsSystem& physics) const
    {
        insertObjectPhysics(ptr, model, rotation, physics);
    }

    void Activator::insertObjectPhysics(const MWWorld::Ptr& ptr, const std::string& model, const osg::Quat& rotation, MWPhysics::PhysicsSystem& physics) const
    {
        physics.addObject(ptr, model, rotation, MWPhysics::CollisionType_World);
    }

    std::string Activator::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return MWBase::Environment::get().getWindowManager()->correctMeshPath(model);
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

    bool Activator::hasToolTip (const MWWorld::ConstPtr& ptr) const
    {
        return !getName(ptr).empty();
    }

    MWGui::ToolTipInfo Activator::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        MWGui::ToolTipInfo info;
        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr)) + MWGui::ToolTips::getCountString(count);

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }

    std::unique_ptr<MWWorld::Action> Activator::activate(const MWWorld::Ptr &ptr, const MWWorld::Ptr &actor) const
    {
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfActivator", prng);

            std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>("#{sWerewolfRefusal}");
            if(sound) action->setSound(sound->mId);

            return action;
        }
        return std::make_unique<MWWorld::NullAction>();
    }


    MWWorld::Ptr Activator::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::Activator> *ref = ptr.get<ESM::Activator>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    std::string Activator::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        const std::string model = getModel(ptr); // Assume it's not empty, since we wouldn't have gotten the soundgen otherwise
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore(); 
        std::string creatureId;

        for (const ESM::Creature &iter : store.get<ESM::Creature>())
        {
            if (!iter.mModel.empty() && Misc::StringUtils::ciEqual(model,
                MWBase::Environment::get().getWindowManager()->correctMeshPath(iter.mModel)))
            {
                creatureId = !iter.mOriginal.empty() ? iter.mOriginal : iter.mId;
                break;
            }
        }

        int type = getSndGenTypeFromName(name);

        std::vector<const ESM::SoundGenerator*> fallbacksounds;
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        if (!creatureId.empty())
        {
            std::vector<const ESM::SoundGenerator*> sounds;
            for (auto sound = store.get<ESM::SoundGenerator>().begin(); sound != store.get<ESM::SoundGenerator>().end(); ++sound)
            {
                if (type == sound->mType && !sound->mCreature.empty() && (Misc::StringUtils::ciEqual(creatureId, sound->mCreature)))
                    sounds.push_back(&*sound);
                if (type == sound->mType && sound->mCreature.empty())
                    fallbacksounds.push_back(&*sound);
            }

            if (!sounds.empty())
                return sounds[Misc::Rng::rollDice(sounds.size(), prng)]->mSound;
            if (!fallbacksounds.empty())
                return fallbacksounds[Misc::Rng::rollDice(fallbacksounds.size(), prng)]->mSound;
        }
        else
        {
            // The activator doesn't have a corresponding creature ID, but we can try to use the defaults
            for (auto sound = store.get<ESM::SoundGenerator>().begin(); sound != store.get<ESM::SoundGenerator>().end(); ++sound)
                if (type == sound->mType && sound->mCreature.empty())
                    fallbacksounds.push_back(&*sound);

            if (!fallbacksounds.empty())
                return fallbacksounds[Misc::Rng::rollDice(fallbacksounds.size(), prng)]->mSound;
        }

        return std::string();
    }

    int Activator::getSndGenTypeFromName(const std::string &name)
    {
        if (name == "left")
            return ESM::SoundGenerator::LeftFoot;
        if (name == "right")
            return ESM::SoundGenerator::RightFoot;
        if (name == "swimleft")
            return ESM::SoundGenerator::SwimLeft;
        if (name == "swimright")
            return ESM::SoundGenerator::SwimRight;
        if (name == "moan")
            return ESM::SoundGenerator::Moan;
        if (name == "roar")
            return ESM::SoundGenerator::Roar;
        if (name == "scream")
            return ESM::SoundGenerator::Scream;
        if (name == "land")
            return ESM::SoundGenerator::Land;

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }
}
