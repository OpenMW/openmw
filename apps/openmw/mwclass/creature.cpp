
#include "creature.hpp"

#include <components/esm/loadcrea.hpp>

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/magiceffects.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/actors.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwmechanics/npcstats.hpp"

namespace
{
    struct CustomData : public MWWorld::CustomData
    {
        MWMechanics::CreatureStats mCreatureStats;
        MWWorld::ContainerStore mContainerStore;
        MWMechanics::Movement mMovement;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        return new CustomData (*this);
    }
}

namespace MWClass
{
    void Creature::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data (new CustomData);

            static bool inited = false;
            if(!inited)
            {
                const MWBase::World *world = MWBase::Environment::get().getWorld();
                const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

                fMinWalkSpeedCreature = gmst.find("fMinWalkSpeedCreature");
                fMaxWalkSpeedCreature = gmst.find("fMaxWalkSpeedCreature");

                inited = true;
            }

            MWWorld::LiveCellRef<ESM::Creature> *ref = ptr.get<ESM::Creature>();

            // creature stats
            data->mCreatureStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mData.mStrength);
            data->mCreatureStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mData.mIntelligence);
            data->mCreatureStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mData.mWillpower);
            data->mCreatureStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mData.mAgility);
            data->mCreatureStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mData.mSpeed);
            data->mCreatureStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mData.mEndurance);
            data->mCreatureStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mData.mPersonality);
            data->mCreatureStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mData.mLuck);
            data->mCreatureStats.setHealth (ref->mBase->mData.mHealth);
            data->mCreatureStats.setMagicka (ref->mBase->mData.mMana);
            data->mCreatureStats.setFatigue (ref->mBase->mData.mFatigue);

            data->mCreatureStats.setLevel(ref->mBase->mData.mLevel);

            data->mCreatureStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
                data->mCreatureStats.getSpells().add (*iter);

            // inventory
            data->mContainerStore.fill(ref->mBase->mInventory, getId(ptr), "",
                                       MWBase::Environment::get().getWorld()->getStore());

            // TODO: this is not quite correct, in vanilla the merchant's gold pool is not available in his inventory.
            // (except for gold you gave him)
            data->mContainerStore.add("gold_001", ref->mBase->mData.mGold, ptr);

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string Creature::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mId;
    }

    void Creature::adjustPosition(const MWWorld::Ptr& ptr) const
    {
        MWBase::Environment::get().getWorld()->adjustPosition(ptr);
    }

    void Creature::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        MWRender::Actors& actors = renderingInterface.getActors();
        actors.insertCreature(ptr);
    }

    void Creature::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addActor(ptr);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Creature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Creature::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mName;
    }

    MWMechanics::CreatureStats& Creature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }


    void Creature::hit(const MWWorld::Ptr& ptr, int type) const
    {
    }

    void Creature::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const
    {
        // NOTE: 'object' and/or 'attacker' may be empty.

        if(!successful)
        {
            // TODO: Handle HitAttemptOnMe script function

            // Missed
            MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitObject(MWWorld::Class::get(object).getId(object));

        if(!attacker.isEmpty() && attacker.getRefData().getHandle() == "player")
        {
            const std::string &script = ptr.get<ESM::Creature>()->mBase->mScript;
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if(ishealth)
        {
            if(damage > 0.0f)
                MWBase::Environment::get().getSoundManager()->playSound3D(ptr, "Health Damage", 1.0f, 1.0f);
            float health = getCreatureStats(ptr).getHealth().getCurrent() - damage;
            setActorHealth(ptr, health, attacker);
        }
        else
        {
            MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
            fatigue.setCurrent(fatigue.getCurrent() - damage, true);
            getCreatureStats(ptr).setFatigue(fatigue);
        }
    }

    void Creature::setActorHealth(const MWWorld::Ptr& ptr, float health, const MWWorld::Ptr& attacker) const
    {
        MWMechanics::CreatureStats &crstats = getCreatureStats(ptr);
        bool wasDead = crstats.isDead();

        MWMechanics::DynamicStat<float> stat(crstats.getHealth());
        stat.setCurrent(health);
        crstats.setHealth(stat);

        if(!wasDead && crstats.isDead())
        {
            // actor was just killed
        }
        else if(wasDead && !crstats.isDead())
        {
            // actor was just resurrected
        }
    }


    boost::shared_ptr<MWWorld::Action> Creature::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if(get(actor).isNpc() && get(actor).getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfCreature");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        if(getCreatureStats(ptr).isDead())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr, true));
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));
    }

    MWWorld::ContainerStore& Creature::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mContainerStore;
    }

    std::string Creature::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mScript;
    }

    bool Creature::isEssential (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Essential;
    }

    void Creature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Creature);

        registerClass (typeid (ESM::Creature).name(), instance);
    }

    bool Creature::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        /// \todo We don't want tooltips for Creatures in combat mode.

        return true;
    }

    float Creature::getSpeed(const MWWorld::Ptr &ptr) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        float walkSpeed = fMinWalkSpeedCreature->getFloat() + 0.01 * stats.getAttribute(ESM::Attribute::Speed).getModified()
                * (fMaxWalkSpeedCreature->getFloat() - fMinWalkSpeedCreature->getFloat());
        /// \todo what about the rest?
        return walkSpeed;
    }

    MWMechanics::Movement& Creature::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 Creature::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mPosition);
        movement.mPosition[0] = 0.0f;
        movement.mPosition[1] = 0.0f;
        movement.mPosition[2] = 0.0f;
        return vec;
    }

    Ogre::Vector3 Creature::getRotationVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mRotation);
        movement.mRotation[0] = 0.0f;
        movement.mRotation[1] = 0.0f;
        movement.mRotation[2] = 0.0f;
        return vec;
    }

    MWGui::ToolTipInfo Creature::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        info.text = text;

        return info;
    }

    float Creature::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        /// \todo add Shield magic effect magnitude here, controlled by a GMST (Vanilla vs MCP behaviour)
        return 0.f;
    }

    float Creature::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return stats.getAttribute(0).getModified()*5;
    }

    float Creature::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Feather)).mMagnitude;

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Burden)).mMagnitude;

        if (weight<0)
            weight = 0;

        return weight;
    }


    int Creature::getServices(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = actor.get<ESM::Creature>();
        if (ref->mBase->mHasAI)
            return ref->mBase->mAiData.mServices;
        else
            return 0;
    }

    bool Creature::isPersistent(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::Creature>* ref = actor.get<ESM::Creature>();
        return ref->mBase->mPersistent;
    }

    std::string Creature::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        const MWWorld::Store<ESM::SoundGenerator> &store = MWBase::Environment::get().getWorld()->getStore().get<ESM::SoundGenerator>();

        int type = getSndGenTypeFromName(ptr, name);
        if(type >= 0)
        {
            std::vector<const ESM::SoundGenerator*> sounds;
            sounds.reserve(8);

            std::string ptrid = Creature::getId(ptr);
            MWWorld::Store<ESM::SoundGenerator>::iterator sound = store.begin();
            while(sound != store.end())
            {
                if(type == sound->mType && !sound->mCreature.empty() &&
                   Misc::StringUtils::ciEqual(ptrid.substr(0, sound->mCreature.size()),
                                              sound->mCreature))
                    sounds.push_back(&*sound);
                ++sound;
            }
            if(!sounds.empty())
                return sounds[(int)(rand()/(RAND_MAX+1.0)*sounds.size())]->mSound;
        }

        return "";
    }

    MWWorld::Ptr
    Creature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return MWWorld::Ptr(&cell.mCreatures.insert(*ref), &cell);
    }

    bool Creature::isFlying(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Creature> *ref =
            ptr.get<ESM::Creature>();

        return ref->mBase->mFlags & ESM::Creature::Flies;
    }

    int Creature::getSndGenTypeFromName(const MWWorld::Ptr &ptr, const std::string &name)
    {
        if(name == "left")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos))
                return 2;
            if(world->isOnGround(ptr))
                return 0;
            return -1;
        }
        if(name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            Ogre::Vector3 pos(ptr.getRefData().getPosition().pos);
            if(world->isUnderwater(ptr.getCell(), pos))
                return 3;
            if(world->isOnGround(ptr))
                return 1;
            return -1;
        }
        if(name == "swimleft")
            return 2;
        if(name == "swimright")
            return 3;
        if(name == "moan")
            return 4;
        if(name == "roar")
            return 5;
        if(name == "scream")
            return 6;
        if(name == "land")
            return 7;

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }

    const ESM::GameSetting* Creature::fMinWalkSpeedCreature;
    const ESM::GameSetting* Creature::fMaxWalkSpeedCreature;
}
