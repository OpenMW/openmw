
#include "npc.hpp"

#include <memory>

#include <boost/algorithm/string.hpp>

#include <OgreSceneNode.h>

#include <components/esm/loadnpc.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/physicssystem.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    const Ogre::Radian kOgrePi (Ogre::Math::PI);
    const Ogre::Radian kOgrePiOverTwo (Ogre::Math::PI / Ogre::Real(2.0));

    struct CustomData : public MWWorld::CustomData
    {
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::CreatureStats mCreatureStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *CustomData::clone() const
    {
        return new CustomData (*this);
    }
}

namespace MWClass
{
    void Npc::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        static bool inited = false;
        if(!inited)
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

            fMinWalkSpeed = gmst.find("fMinWalkSpeed");
            fMaxWalkSpeed = gmst.find("fMaxWalkSpeed");
            fEncumberedMoveEffect = gmst.find("fEncumberedMoveEffect");
            fSneakSpeedMultiplier = gmst.find("fSneakSpeedMultiplier");
            fAthleticsRunBonus = gmst.find("fAthleticsRunBonus");
            fBaseRunMultiplier = gmst.find("fBaseRunMultiplier");
            fMinFlySpeed = gmst.find("fMinFlySpeed");
            fMaxFlySpeed = gmst.find("fMaxFlySpeed");
            fSwimRunBase = gmst.find("fSwimRunBase");
            fSwimRunAthleticsMult = gmst.find("fSwimRunAthleticsMult");
            fJumpEncumbranceBase = gmst.find("fJumpEncumbranceBase");
            fJumpEncumbranceMultiplier = gmst.find("fJumpEncumbranceMultiplier");
            fJumpAcrobaticsBase = gmst.find("fJumpAcrobaticsBase");
            fJumpAcroMultiplier = gmst.find("fJumpAcroMultiplier");
            fJumpRunMultiplier = gmst.find("fJumpRunMultiplier");
            // Added in Tribunal/Bloodmoon, may not exist
            fWereWolfRunMult = gmst.search("fWereWolfRunMult");

            inited = true;
        }
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<CustomData> data(new CustomData);

            MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

            // NPC stats
            if (!ref->mBase->mFaction.empty())
            {
                std::string faction = ref->mBase->mFaction;
                Misc::StringUtils::toLower(faction);
                if(ref->mBase->mNpdt52.mGold != -10)
                {
                    data->mNpcStats.getFactionRanks()[faction] = (int)ref->mBase->mNpdt52.mRank;
                }
                else
                {
                    data->mNpcStats.getFactionRanks()[faction] = (int)ref->mBase->mNpdt12.mRank;
                }
            }

            // creature stats
            if(ref->mBase->mNpdt52.mGold != -10)
            {
                for (int i=0; i<27; ++i)
                    data->mNpcStats.getSkill (i).setBase (ref->mBase->mNpdt52.mSkills[i]);

                data->mCreatureStats.getAttribute(0).set (ref->mBase->mNpdt52.mStrength);
                data->mCreatureStats.getAttribute(1).set (ref->mBase->mNpdt52.mIntelligence);
                data->mCreatureStats.getAttribute(2).set (ref->mBase->mNpdt52.mWillpower);
                data->mCreatureStats.getAttribute(3).set (ref->mBase->mNpdt52.mAgility);
                data->mCreatureStats.getAttribute(4).set (ref->mBase->mNpdt52.mSpeed);
                data->mCreatureStats.getAttribute(5).set (ref->mBase->mNpdt52.mEndurance);
                data->mCreatureStats.getAttribute(6).set (ref->mBase->mNpdt52.mPersonality);
                data->mCreatureStats.getAttribute(7).set (ref->mBase->mNpdt52.mLuck);
                data->mCreatureStats.setHealth (ref->mBase->mNpdt52.mHealth);
                data->mCreatureStats.setMagicka (ref->mBase->mNpdt52.mMana);
                data->mCreatureStats.setFatigue (ref->mBase->mNpdt52.mFatigue);

                data->mCreatureStats.setLevel(ref->mBase->mNpdt52.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt52.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt52.mReputation);
            }
            else
            {
                for (int i=0; i<8; ++i)
                    data->mCreatureStats.getAttribute (i).set (10);

                for (int i=0; i<3; ++i)
                    data->mCreatureStats.setDynamic (i, 10);

                data->mCreatureStats.setLevel(ref->mBase->mNpdt12.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt12.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt12.mReputation);
            }

            data->mCreatureStats.setAiSetting (0, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting (1, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting (2, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting (3, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
                data->mCreatureStats.getSpells().add (*iter);

            // store
            ptr.getRefData().setCustomData (data.release());
        }
    }

    std::string Npc::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mId;
    }

    void Npc::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getActors().insertNPC(ptr, getInventoryStore(ptr));
    }

    void Npc::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        physics.addActor(ptr);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
    }

    std::string Npc::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();
        assert(ref->mBase != NULL);

        std::string headID = ref->mBase->mHead;

        int end = headID.find_last_of("head_") - 4;
        std::string bodyRaceID = headID.substr(0, end);

        std::string model = "meshes\\base_anim.nif";
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
        if(race->mData.mFlags & ESM::Race::Beast)
            model = "meshes\\base_animkna.nif";

        return model;

    }

    std::string Npc::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mName;
    }

    MWMechanics::CreatureStats& Npc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }

    MWMechanics::NpcStats& Npc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    boost::shared_ptr<MWWorld::Action> Npc::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        if (MWWorld::Class::get (ptr).getCreatureStats (ptr).isDead())
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionOpen(ptr, true));
        else if (MWWorld::Class::get(actor).getStance(actor, MWWorld::Class::Sneak))
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionOpen(ptr)); // stealing
        else
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::ActionTalk (ptr));
    }

    MWWorld::ContainerStore& Npc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWWorld::InventoryStore& Npc::getInventoryStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    std::string Npc::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mScript;
    }

    void Npc::setForceStance (const MWWorld::Ptr& ptr, Stance stance, bool force) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_ForceRun, force);
                break;

            case Sneak:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak, force);
                break;

            case Combat:

                throw std::runtime_error ("combat stance not enforcable for NPCs");
        }
    }

    void Npc::setStance (const MWWorld::Ptr& ptr, Stance stance, bool set) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_Run, set);
                break;

            case Sneak:

                stats.setMovementFlag (MWMechanics::NpcStats::Flag_Sneak, set);
                break;

            case Combat:

                // Combat stance ignored for now; need to be determined based on draw state instead of
                // being maunally set.
                break;
        }
    }

    bool Npc::getStance (const MWWorld::Ptr& ptr, Stance stance, bool ignoreForce) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        switch (stance)
        {
            case Run:

                if (!ignoreForce && stats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceRun))
                    return true;

                return stats.getMovementFlag (MWMechanics::NpcStats::Flag_Run);

            case Sneak:

                if (!ignoreForce && stats.getMovementFlag (MWMechanics::NpcStats::Flag_ForceSneak))
                    return true;

                return stats.getMovementFlag (MWMechanics::NpcStats::Flag_Sneak);

            case Combat:

                return false;
        }

        return false;
    }

    float Npc::getSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const CustomData *npcdata = static_cast<const CustomData*>(ptr.getRefData().getCustomData());
        const MWMechanics::MagicEffects &mageffects = npcdata->mCreatureStats.getMagicEffects();

        const float normalizedEncumbrance = Npc::getEncumbrance(ptr) / Npc::getCapacity(ptr);

        float walkSpeed = fMinWalkSpeed->getFloat() + 0.01f*npcdata->mCreatureStats.getAttribute(ESM::Attribute::Speed).getModified()*
                                                      (fMaxWalkSpeed->getFloat() - fMinWalkSpeed->getFloat());
        walkSpeed *= 1.0f - fEncumberedMoveEffect->getFloat()*normalizedEncumbrance;
        walkSpeed = std::max(0.0f, walkSpeed);
        if(Npc::getStance(ptr, Sneak, false))
            walkSpeed *= fSneakSpeedMultiplier->getFloat();

        float runSpeed = walkSpeed*(0.01f * npcdata->mNpcStats.getSkill(ESM::Skill::Athletics).getModified() *
                                    fAthleticsRunBonus->getFloat() + fBaseRunMultiplier->getFloat());
        if(npcdata->mNpcStats.isWerewolf())
            runSpeed *= fWereWolfRunMult->getFloat();

        float moveSpeed;
        if(normalizedEncumbrance >= 1.0f)
            moveSpeed = 0.0f;
        else if(mageffects.get(MWMechanics::EffectKey(10/*levitate*/)).mMagnitude > 0)
        {
            float flySpeed = 0.01f*(npcdata->mCreatureStats.getAttribute(ESM::Attribute::Speed).getModified() +
                                    mageffects.get(MWMechanics::EffectKey(10/*levitate*/)).mMagnitude);
            flySpeed = fMinFlySpeed->getFloat() + flySpeed*(fMaxFlySpeed->getFloat() - fMinFlySpeed->getFloat());
            flySpeed *= 1.0f - fEncumberedMoveEffect->getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if(world->isSwimming(ptr))
        {
            float swimSpeed = walkSpeed;
            if(Npc::getStance(ptr, Run, false))
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(MWMechanics::EffectKey(1/*swift swim*/)).mMagnitude;
            swimSpeed *= fSwimRunBase->getFloat() + 0.01f*npcdata->mNpcStats.getSkill(ESM::Skill::Athletics).getModified()*
                                                    fSwimRunAthleticsMult->getFloat();
            moveSpeed = swimSpeed;
        }
        else if(Npc::getStance(ptr, Run, false) && !Npc::getStance(ptr, Sneak, false))
            moveSpeed = runSpeed;
        else
            moveSpeed = walkSpeed;
        if(getMovementSettings(ptr).mLeftRight != 0 && getMovementSettings(ptr).mForwardBackward == 0)
            moveSpeed *= 0.75f;

        return moveSpeed;
    }

    float Npc::getJump(const MWWorld::Ptr &ptr) const
    {
        const CustomData *npcdata = static_cast<const CustomData*>(ptr.getRefData().getCustomData());
        const MWMechanics::MagicEffects &mageffects = npcdata->mCreatureStats.getMagicEffects();
        const float encumbranceTerm = fJumpEncumbranceBase->getFloat() +
                                          fJumpEncumbranceMultiplier->getFloat() *
                                          (1.0f - Npc::getEncumbrance(ptr)/Npc::getCapacity(ptr));

        float a = npcdata->mNpcStats.getSkill(ESM::Skill::Acrobatics).getModified();
        float b = 0.0f;
        if(a > 50.0f)
        {
            b = a - 50.0f;
            a = 50.0f;
        }

        float x = fJumpAcrobaticsBase->getFloat() +
                  std::pow(a / 15.0f, fJumpAcroMultiplier->getFloat());
        x += 3 * b * fJumpAcroMultiplier->getFloat();
        x += mageffects.get(MWMechanics::EffectKey(9/*jump*/)).mMagnitude * 64;
        x *= encumbranceTerm;

        if(Npc::getStance(ptr, Run, false))
            x *= fJumpRunMultiplier->getFloat();
        x *= 1.25f;//fatigueTerm;
        x -= -627.2/*gravity constant*/;
        x /= 3;

        return x;
    }

    MWMechanics::Movement& Npc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<CustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 Npc::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        Ogre::Vector3 vector;
        vector.x = getMovementSettings(ptr).mLeftRight;
        vector.y = getMovementSettings(ptr).mForwardBackward;
        vector.z = getMovementSettings(ptr).mUpDown;

        return vector;
    }
    
    bool Npc::isEssential (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mFlags & ESM::NPC::Essential;
    }
    
    void Npc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Npc);
        registerClass (typeid (ESM::NPC).name(), instance);
    }

    bool Npc::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        /// \todo We don't want tooltips for NPCs in combat mode.

        return true;
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName;

        std::string text;
        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        info.text = text;

        return info;
    }

    float Npc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        return stats.getAttribute(0).getModified()*5;
    }

    float Npc::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        float weight = getContainerStore (ptr).getWeight();

        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        weight -= stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Feather)).mMagnitude;

        weight += stats.getMagicEffects().get (MWMechanics::EffectKey (ESM::MagicEffect::Burden)).mMagnitude;

        if (weight<0)
            weight = 0;

        return weight;
    }

    bool Npc::apply (const MWWorld::Ptr& ptr, const std::string& id,
        const MWWorld::Ptr& actor) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats (ptr);

        /// \todo consider instant effects

        return stats.getActiveSpells().addSpell (id, actor);
    }

    void Npc::skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        const ESM::Class *class_ =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (
                ref->mBase->mClass
            );

        stats.useSkill (skill, *class_, usageType);
    }

    float Npc::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        MWWorld::InventoryStore& invStore = MWWorld::Class::get(ptr).getInventoryStore(ptr);
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        int ratings[MWWorld::InventoryStore::Slots];

        int iBaseArmorSkill = gmst.find("iBaseArmorSkill")->getInt();
        float fUnarmoredBase1 = gmst.find("fUnarmoredBase1")->getFloat();
        float fUnarmoredBase2 = gmst.find("fUnarmoredBase2")->getFloat();
        int unarmoredSkill = MWWorld::Class::get(ptr).getNpcStats(ptr).getSkill(ESM::Skill::Unarmored).getModified();

        for (int i = 0; i < MWWorld::InventoryStore::Slots; ++i)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getTypeName() != typeid(ESM::Armor).name())
            {
                // unarmored
                ratings[i] = (fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill);
            }
            else
            {
                MWWorld::LiveCellRef<ESM::Armor> *ref =
                    it->get<ESM::Armor>();

                int armorSkillType = MWWorld::Class::get(*it).getEquipmentSkill(*it);
                int armorSkill = MWWorld::Class::get(ptr).getNpcStats(ptr).getSkill(armorSkillType).getModified();

                if (ref->mBase->mData.mWeight == 0)
                    ratings[i] = ref->mBase->mData.mArmor;
                else
                    ratings[i] = ref->mBase->mData.mArmor * armorSkill / iBaseArmorSkill;
            }
        }

        float shield = MWWorld::Class::get(ptr).getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Shield).mMagnitude;

        return ratings[MWWorld::InventoryStore::Slot_Cuirass] * 0.3
                + (ratings[MWWorld::InventoryStore::Slot_CarriedLeft] + ratings[MWWorld::InventoryStore::Slot_Helmet]
                    + ratings[MWWorld::InventoryStore::Slot_Greaves] + ratings[MWWorld::InventoryStore::Slot_Boots]
                    + ratings[MWWorld::InventoryStore::Slot_LeftPauldron] + ratings[MWWorld::InventoryStore::Slot_RightPauldron]
                    ) * 0.1
                + (ratings[MWWorld::InventoryStore::Slot_LeftGauntlet] + MWWorld::InventoryStore::Slot_RightGauntlet)
                    * 0.05
                + shield;
    }


    void Npc::adjustRotation(const MWWorld::Ptr& ptr,float& x,float& y,float& z) const
    {
        y = 0;
        x = 0;
    }

    void Npc::adjustScale(const MWWorld::Ptr &ptr, float &scale) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        const ESM::Race* race =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);

        if (ref->mBase->isMale())
            scale *= race->mData.mHeight.mMale;
        else
            scale *= race->mData.mHeight.mFemale;
    }

    MWWorld::Ptr
    Npc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return MWWorld::Ptr(&cell.mNpcs.insert(*ref), &cell);
    }

    const ESM::GameSetting *Npc::fMinWalkSpeed;
    const ESM::GameSetting *Npc::fMaxWalkSpeed;
    const ESM::GameSetting *Npc::fEncumberedMoveEffect;
    const ESM::GameSetting *Npc::fSneakSpeedMultiplier;
    const ESM::GameSetting *Npc::fAthleticsRunBonus;
    const ESM::GameSetting *Npc::fBaseRunMultiplier;
    const ESM::GameSetting *Npc::fMinFlySpeed;
    const ESM::GameSetting *Npc::fMaxFlySpeed;
    const ESM::GameSetting *Npc::fSwimRunBase;
    const ESM::GameSetting *Npc::fSwimRunAthleticsMult;
    const ESM::GameSetting *Npc::fJumpEncumbranceBase;
    const ESM::GameSetting *Npc::fJumpEncumbranceMultiplier;
    const ESM::GameSetting *Npc::fJumpAcrobaticsBase;
    const ESM::GameSetting *Npc::fJumpAcroMultiplier;
    const ESM::GameSetting *Npc::fJumpRunMultiplier;
    const ESM::GameSetting *Npc::fWereWolfRunMult;
}
