#include "npc.hpp"

#include <memory>

#include <components/misc/rng.hpp>

#include <components/esm/loadmgef.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/npcstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/disease.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/autocalcspell.hpp"
#include "../mwmechanics/difficultyscaling.hpp"
#include "../mwmechanics/character.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    struct NpcCustomData : public MWWorld::CustomData
    {
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *NpcCustomData::clone() const
    {
        return new NpcCustomData (*this);
    }

    int is_even(double d) {
        double int_part;
        modf(d / 2.0, &int_part);
        return 2.0 * int_part == d;
    }

    int round_ieee_754(double d) {
        double i = floor(d);
        d -= i;
        if(d < 0.5)
            return static_cast<int>(i);
        if(d > 0.5)
            return static_cast<int>(i) + 1;
        if(is_even(i))
            return static_cast<int>(i);
        return static_cast<int>(i) + 1;
    }

    void autoCalculateAttributes (const ESM::NPC* npc, MWMechanics::CreatureStats& creatureStats)
    {
        // race bonus
        const ESM::Race *race =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npc->mRace);

        bool male = (npc->mFlags & ESM::NPC::Female) == 0;

        int level = creatureStats.getLevel();
        for (int i=0; i<ESM::Attribute::Length; ++i)
        {
            const ESM::Race::MaleFemale& attribute = race->mData.mAttributeValues[i];
            creatureStats.setAttribute(i, male ? attribute.mMale : attribute.mFemale);
        }

        // class bonus
        const ESM::Class *class_ =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(npc->mClass);

        for (int i=0; i<2; ++i)
        {
            int attribute = class_->mData.mAttribute[i];
            if (attribute>=0 && attribute<8)
            {
                creatureStats.setAttribute(attribute,
                    creatureStats.getAttribute(attribute).getBase() + 10);
            }
        }

        // skill bonus
        for (int attribute=0; attribute < ESM::Attribute::Length; ++attribute)
        {
            float modifierSum = 0;

            for (int j=0; j<ESM::Skill::Length; ++j)
            {
                const ESM::Skill* skill = MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find(j);

                if (skill->mData.mAttribute != attribute)
                    continue;

                // is this a minor or major skill?
                float add=0.2f;
                for (int k=0; k<5; ++k)
                {
                    if (class_->mData.mSkills[k][0] == j)
                        add=0.5;
                }
                for (int k=0; k<5; ++k)
                {
                    if (class_->mData.mSkills[k][1] == j)
                        add=1.0;
                }
                modifierSum += add;
            }
            creatureStats.setAttribute(attribute, std::min(
                                           round_ieee_754(creatureStats.getAttribute(attribute).getBase()
                + (level-1) * modifierSum), 100) );
        }

        // initial health
        int strength = creatureStats.getAttribute(ESM::Attribute::Strength).getBase();
        int endurance = creatureStats.getAttribute(ESM::Attribute::Endurance).getBase();

        int multiplier = 3;

        if (class_->mData.mSpecialization == ESM::Class::Combat)
            multiplier += 2;
        else if (class_->mData.mSpecialization == ESM::Class::Stealth)
            multiplier += 1;

        if (class_->mData.mAttribute[0] == ESM::Attribute::Endurance
            || class_->mData.mAttribute[1] == ESM::Attribute::Endurance)
            multiplier += 1;

        creatureStats.setHealth(floor(0.5f * (strength + endurance)) + multiplier * (creatureStats.getLevel() - 1));
    }

    /**
     * @brief autoCalculateSkills
     *
     * Skills are calculated with following formulae ( http://www.uesp.net/wiki/Morrowind:NPCs#Skills ):
     *
     * Skills: (Level - 1) × (Majority Multiplier + Specialization Multiplier)
     *
     *         The Majority Multiplier is 1.0 for a Major or Minor Skill, or 0.1 for a Miscellaneous Skill.
     *
     *         The Specialization Multiplier is 0.5 for a Skill in the same Specialization as the class,
     *         zero for other Skills.
     *
     * and by adding class, race, specialization bonus.
     */
    void autoCalculateSkills(const ESM::NPC* npc, MWMechanics::NpcStats& npcStats, const MWWorld::Ptr& ptr)
    {
        const ESM::Class *class_ =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find(npc->mClass);

        unsigned int level = npcStats.getLevel();

        const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npc->mRace);


        for (int i = 0; i < 2; ++i)
        {
            int bonus = (i==0) ? 10 : 25;

            for (int i2 = 0; i2 < 5; ++i2)
            {
                int index = class_->mData.mSkills[i2][i];
                if (index >= 0 && index < ESM::Skill::Length)
                {
                    npcStats.getSkill(index).setBase (npcStats.getSkill(index).getBase() + bonus);
                }
            }
        }

        for (int skillIndex = 0; skillIndex < ESM::Skill::Length; ++skillIndex)
        {
            float majorMultiplier = 0.1f;
            float specMultiplier = 0.0f;

            int raceBonus = 0;
            int specBonus = 0;

            for (int raceSkillIndex = 0; raceSkillIndex < 7; ++raceSkillIndex)
            {
                if (race->mData.mBonus[raceSkillIndex].mSkill == skillIndex)
                {
                    raceBonus = race->mData.mBonus[raceSkillIndex].mBonus;
                    break;
                }
            }

            for (int k = 0; k < 5; ++k)
            {
                // is this a minor or major skill?
                if ((class_->mData.mSkills[k][0] == skillIndex) || (class_->mData.mSkills[k][1] == skillIndex))
                {
                    majorMultiplier = 1.0f;
                    break;
                }
            }

            // is this skill in the same Specialization as the class?
            const ESM::Skill* skill = MWBase::Environment::get().getWorld()->getStore().get<ESM::Skill>().find(skillIndex);
            if (skill->mData.mSpecialization == class_->mData.mSpecialization)
            {
                specMultiplier = 0.5f;
                specBonus = 5;
            }

            npcStats.getSkill(skillIndex).setBase(
                  std::min(
                    round_ieee_754(
                            npcStats.getSkill(skillIndex).getBase()
                    + 5
                    + raceBonus
                    + specBonus
                    +(int(level)-1) * (majorMultiplier + specMultiplier)), 100)); // Must gracefully handle level 0
        }

        int skills[ESM::Skill::Length];
        for (int i=0; i<ESM::Skill::Length; ++i)
            skills[i] = npcStats.getSkill(i).getBase();

        int attributes[ESM::Attribute::Length];
        for (int i=0; i<ESM::Attribute::Length; ++i)
            attributes[i] = npcStats.getAttribute(i).getBase();

        std::vector<std::string> spells = MWMechanics::autoCalcNpcSpells(skills, attributes, race);
        for (std::vector<std::string>::iterator it = spells.begin(); it != spells.end(); ++it)
            npcStats.getSpells().add(*it);
    }
}

namespace MWClass
{
    const Npc::GMST& Npc::getGmst()
    {
        static GMST gmst;
        static bool inited = false;
        if(!inited)
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

            gmst.fMinWalkSpeed = store.find("fMinWalkSpeed");
            gmst.fMaxWalkSpeed = store.find("fMaxWalkSpeed");
            gmst.fEncumberedMoveEffect = store.find("fEncumberedMoveEffect");
            gmst.fSneakSpeedMultiplier = store.find("fSneakSpeedMultiplier");
            gmst.fAthleticsRunBonus = store.find("fAthleticsRunBonus");
            gmst.fBaseRunMultiplier = store.find("fBaseRunMultiplier");
            gmst.fMinFlySpeed = store.find("fMinFlySpeed");
            gmst.fMaxFlySpeed = store.find("fMaxFlySpeed");
            gmst.fSwimRunBase = store.find("fSwimRunBase");
            gmst.fSwimRunAthleticsMult = store.find("fSwimRunAthleticsMult");
            gmst.fJumpEncumbranceBase = store.find("fJumpEncumbranceBase");
            gmst.fJumpEncumbranceMultiplier = store.find("fJumpEncumbranceMultiplier");
            gmst.fJumpAcrobaticsBase = store.find("fJumpAcrobaticsBase");
            gmst.fJumpAcroMultiplier = store.find("fJumpAcroMultiplier");
            gmst.fJumpRunMultiplier = store.find("fJumpRunMultiplier");
            gmst.fWereWolfRunMult = store.find("fWereWolfRunMult");
            gmst.fKnockDownMult = store.find("fKnockDownMult");
            gmst.iKnockDownOddsMult = store.find("iKnockDownOddsMult");
            gmst.iKnockDownOddsBase = store.find("iKnockDownOddsBase");
            gmst.fCombatArmorMinMult = store.find("fCombatArmorMinMult");

            inited = true;
        }
        return gmst;
    }

    void Npc::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<NpcCustomData> data(new NpcCustomData);

            MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

            // creature stats
            int gold=0;
            if(ref->mBase->mNpdtType != ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
            {
                gold = ref->mBase->mNpdt52.mGold;

                for (unsigned int i=0; i< ESM::Skill::Length; ++i)
                    data->mNpcStats.getSkill (i).setBase (ref->mBase->mNpdt52.mSkills[i]);

                data->mNpcStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mNpdt52.mStrength);
                data->mNpcStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mNpdt52.mIntelligence);
                data->mNpcStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mNpdt52.mWillpower);
                data->mNpcStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mNpdt52.mAgility);
                data->mNpcStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mNpdt52.mSpeed);
                data->mNpcStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mNpdt52.mEndurance);
                data->mNpcStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mNpdt52.mPersonality);
                data->mNpcStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mNpdt52.mLuck);

                data->mNpcStats.setHealth (ref->mBase->mNpdt52.mHealth);
                data->mNpcStats.setMagicka (ref->mBase->mNpdt52.mMana);
                data->mNpcStats.setFatigue (ref->mBase->mNpdt52.mFatigue);

                data->mNpcStats.setLevel(ref->mBase->mNpdt52.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt52.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt52.mReputation);

                data->mNpcStats.setNeedRecalcDynamicStats(false);
            }
            else
            {
                gold = ref->mBase->mNpdt12.mGold;

                for (int i=0; i<3; ++i)
                    data->mNpcStats.setDynamic (i, 10);

                data->mNpcStats.setLevel(ref->mBase->mNpdt12.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt12.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt12.mReputation);

                autoCalculateAttributes(ref->mBase, data->mNpcStats);
                autoCalculateSkills(ref->mBase, data->mNpcStats, ptr);

                data->mNpcStats.setNeedRecalcDynamicStats(true);
            }

            // race powers
            const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
            for (std::vector<std::string>::const_iterator iter (race->mPowers.mList.begin());
                iter!=race->mPowers.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                    std::cerr << "Warning: ignoring nonexistent race power '" << *iter << "' on NPC '" << ref->mBase->mId << "'" << std::endl;
            }

            if (!ref->mBase->mFaction.empty())
            {
                static const int iAutoRepFacMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepFacMod")->getInt();
                static const int iAutoRepLevMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepLevMod")->getInt();
                int rank = ref->mBase->getFactionRank();

                data->mNpcStats.setReputation(iAutoRepFacMod * (rank+1) + iAutoRepLevMod * (data->mNpcStats.getLevel()-1));
            }

            data->mNpcStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                {
                    /// \todo add option to make this a fatal error message pop-up, but default to warning for vanilla compatibility
                    std::cerr << "Warning: ignoring nonexistent spell '" << *iter << "' on NPC '" << ref->mBase->mId << "'" << std::endl;
                }
            }

            // inventory
            // setting ownership is used to make the NPC auto-equip his initial equipment only, and not bartered items
            data->mInventoryStore.fill(ref->mBase->mInventory, getId(ptr));

            data->mNpcStats.setGoldPool(gold);

            // store
            ptr.getRefData().setCustomData (data.release());

            getInventoryStore(ptr).autoEquip(ptr); 
        }
    }

    std::string Npc::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mId;
    }

    void Npc::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getObjects().insertNPC(ptr);
    }

    bool Npc::isPersistent(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::NPC>* ref = actor.get<ESM::NPC>();
        return ref->mBase->mPersistent;
    }

    std::string Npc::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();
        assert(ref->mBase != NULL);

        std::string model = "meshes\\base_anim.nif";
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
        if(race->mData.mFlags & ESM::Race::Beast)
            model = "meshes\\base_animkna.nif";

        return model;

    }

    std::string Npc::getName (const MWWorld::Ptr& ptr) const
    {
        if(getNpcStats(ptr).isWerewolf())
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

            return store.find("sWerewolfPopup")->getString();
        }

        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mName;
    }

    MWMechanics::CreatureStats& Npc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    MWMechanics::NpcStats& Npc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }


    void Npc::hit(const MWWorld::Ptr& ptr, float attackStrength, int type) const
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();

        const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::InventoryStore &inv = getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        MWWorld::Ptr weapon = ((weaponslot != inv.end()) ? *weaponslot : MWWorld::Ptr());
        if(!weapon.isEmpty() && weapon.getTypeName() != typeid(ESM::Weapon).name())
            weapon = MWWorld::Ptr();

        MWMechanics::applyFatigueLoss(ptr, weapon, attackStrength);

        const float fCombatDistance = store.find("fCombatDistance")->getFloat();
        float dist = fCombatDistance * (!weapon.isEmpty() ?
                               weapon.get<ESM::Weapon>()->mBase->mData.mReach :
                               store.find("fHandToHandReach")->getFloat());

        // TODO: Use second to work out the hit angle
        std::pair<MWWorld::Ptr, osg::Vec3f> result = world->getHitContact(ptr, dist);
        MWWorld::Ptr victim = result.first;
        osg::Vec3f hitPosition (result.second);
        if(victim.isEmpty()) // Didn't hit anything
            return;

        const MWWorld::Class &othercls = victim.getClass();
        if(!othercls.isActor()) // Can't hit non-actors
            return;
        MWMechanics::CreatureStats &otherstats = othercls.getCreatureStats(victim);
        if(otherstats.isDead()) // Can't hit dead actors
            return;

        if(ptr == MWMechanics::getPlayer())
            MWBase::Environment::get().getWindowManager()->setEnemy(victim);

        int weapskill = ESM::Skill::HandToHand;
        if(!weapon.isEmpty())
            weapskill = weapon.getClass().getEquipmentSkill(weapon);

        float hitchance = MWMechanics::getHitChance(ptr, victim, ptr.getClass().getSkill(ptr, weapskill));

        if (Misc::Rng::roll0to99() >= hitchance)
        {
            othercls.onHit(victim, 0.0f, false, weapon, ptr, false);
            MWMechanics::reduceWeaponCondition(0.f, false, weapon, ptr);
            return;
        }

        bool healthdmg;
        float damage = 0.0f;
        if(!weapon.isEmpty())
        {
            const unsigned char *attack = NULL;
            if(type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if(type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if(type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if(attack)
            {
                damage  = attack[0] + ((attack[1]-attack[0])*attackStrength);
            }
            MWMechanics::adjustWeaponDamage(damage, weapon, ptr);
            MWMechanics::reduceWeaponCondition(damage, true, weapon, ptr);
            healthdmg = true;
        }
        else
        {
            MWMechanics::getHandToHandDamage(ptr, victim, damage, healthdmg, attackStrength);
        }
        if(ptr == MWMechanics::getPlayer())
        {
            skillUsageSucceeded(ptr, weapskill, 0);

            const MWMechanics::AiSequence& seq = victim.getClass().getCreatureStats(victim).getAiSequence();

            bool unaware = !seq.isInCombat()
                    && !MWBase::Environment::get().getMechanicsManager()->awarenessCheck(ptr, victim);
            if(unaware)
            {
                damage *= store.find("fCombatCriticalStrikeMult")->getFloat();
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTargetCriticalStrike}");
                MWBase::Environment::get().getSoundManager()->playSound3D(victim, "critical damage", 1.0f, 1.0f);
            }
        }

        if (othercls.getCreatureStats(victim).getKnockedDown())
            damage *= store.find("fCombatKODamageMult")->getFloat();

        // Apply "On hit" enchanted weapons
        std::string enchantmentName = !weapon.isEmpty() ? weapon.getClass().getEnchantment(weapon) : "";
        if (!enchantmentName.empty())
        {
            const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                        enchantmentName);
            if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
            {
                MWMechanics::CastSpell cast(ptr, victim);
                cast.mHitPosition = hitPosition;
                cast.cast(weapon);
            }
        }

        MWMechanics::applyElementalShields(ptr, victim);

        if (MWMechanics::blockMeleeAttack(ptr, victim, weapon, damage, attackStrength))
            damage = 0;

        if (healthdmg && damage > 0)
            MWBase::Environment::get().getWorld()->spawnBloodEffect(victim, hitPosition);

        MWMechanics::diseaseContact(victim, ptr);

        othercls.onHit(victim, damage, healthdmg, weapon, ptr, true);
    }

    void Npc::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, bool successful) const
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();

        // NOTE: 'object' and/or 'attacker' may be empty.

        bool wasDead = getCreatureStats(ptr).isDead();

        // Note OnPcHitMe is not set for friendly hits.
        bool setOnPcHitMe = true;
        if (!attacker.isEmpty() && !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat(attacker))
        {
            getCreatureStats(ptr).setAttacked(true);

            setOnPcHitMe = MWBase::Environment::get().getMechanicsManager()->actorAttacked(ptr, attacker);
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitAttemptObject(object.getClass().getId(object));

        if(setOnPcHitMe && !attacker.isEmpty() && attacker == MWMechanics::getPlayer())
        {
            const std::string &script = ptr.getClass().getScript(ptr);
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if(!successful)
        {
            // Missed
            sndMgr->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if(!object.isEmpty())
            getCreatureStats(ptr).setLastHitObject(object.getClass().getId(object));


        if (damage > 0.0f && !object.isEmpty())
            MWMechanics::resistNormalWeapon(ptr, attacker, object, damage);

        if (damage < 0.001f)
            damage = 0;

        if(damage > 0.0f && !attacker.isEmpty())
        {
            // 'ptr' is losing health. Play a 'hit' voiced dialog entry if not already saying
            // something, alert the character controller, scripts, etc.

            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const GMST& gmst = getGmst();

            int chance = store.get<ESM::GameSetting>().find("iVoiceHitOdds")->getInt();
            if (Misc::Rng::roll0to99() < chance)
            {
                MWBase::Environment::get().getDialogueManager()->say(ptr, "hit");
            }

            // Check for knockdown
            float agilityTerm = getCreatureStats(ptr).getAttribute(ESM::Attribute::Agility).getModified() * gmst.fKnockDownMult->getFloat();
            float knockdownTerm = getCreatureStats(ptr).getAttribute(ESM::Attribute::Agility).getModified()
                    * gmst.iKnockDownOddsMult->getInt() * 0.01f + gmst.iKnockDownOddsBase->getInt();
            if (ishealth && agilityTerm <= damage && knockdownTerm <= Misc::Rng::roll0to99())
            {
                getCreatureStats(ptr).setKnockedDown(true);

            }
            else
                getCreatureStats(ptr).setHitRecovery(true); // Is this supposed to always occur?

            if(damage > 0 && ishealth)
            {
                // Hit percentages:
                // cuirass = 30%
                // shield, helmet, greaves, boots, pauldrons = 10% each
                // guantlets = 5% each
                static const int hitslots[20] = {
                    MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_CarriedLeft, MWWorld::InventoryStore::Slot_CarriedLeft,
                    MWWorld::InventoryStore::Slot_Helmet, MWWorld::InventoryStore::Slot_Helmet,
                    MWWorld::InventoryStore::Slot_Greaves, MWWorld::InventoryStore::Slot_Greaves,
                    MWWorld::InventoryStore::Slot_Boots, MWWorld::InventoryStore::Slot_Boots,
                    MWWorld::InventoryStore::Slot_LeftPauldron, MWWorld::InventoryStore::Slot_LeftPauldron,
                    MWWorld::InventoryStore::Slot_RightPauldron, MWWorld::InventoryStore::Slot_RightPauldron,
                    MWWorld::InventoryStore::Slot_LeftGauntlet, MWWorld::InventoryStore::Slot_RightGauntlet
                };
                int hitslot = hitslots[Misc::Rng::rollDice(20)];

                float unmitigatedDamage = damage;
                float x = damage / (damage + getArmorRating(ptr));
                damage *= std::max(gmst.fCombatArmorMinMult->getFloat(), x);
                int damageDiff = static_cast<int>(unmitigatedDamage - damage);
                if (damage < 1)
                    damage = 1;

                MWWorld::InventoryStore &inv = getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator armorslot = inv.getSlot(hitslot);
                MWWorld::Ptr armor = ((armorslot != inv.end()) ? *armorslot : MWWorld::Ptr());
                if(!armor.isEmpty() && armor.getTypeName() == typeid(ESM::Armor).name())
                {
                    int armorhealth = armor.getClass().getItemHealth(armor);
                    armorhealth -= std::min(std::max(1, damageDiff),
                                                 armorhealth);
                    armor.getCellRef().setCharge(armorhealth);

                    // Armor broken? unequip it
                    if (armorhealth == 0)
                        armor = *inv.unequipItem(armor, ptr);

                    if (ptr == MWMechanics::getPlayer())
                        skillUsageSucceeded(ptr, armor.getClass().getEquipmentSkill(armor), 0);

                    switch(armor.getClass().getEquipmentSkill(armor))
                    {
                        case ESM::Skill::LightArmor:
                            sndMgr->playSound3D(ptr, "Light Armor Hit", 1.0f, 1.0f);
                            break;
                        case ESM::Skill::MediumArmor:
                            sndMgr->playSound3D(ptr, "Medium Armor Hit", 1.0f, 1.0f);
                            break;
                        case ESM::Skill::HeavyArmor:
                            sndMgr->playSound3D(ptr, "Heavy Armor Hit", 1.0f, 1.0f);
                            break;
                    }
                }
                else if(ptr == MWMechanics::getPlayer())
                    skillUsageSucceeded(ptr, ESM::Skill::Unarmored, 0);
            }
        }

        if(ishealth)
        {
            if (!attacker.isEmpty())
                damage = scaleDamage(damage, attacker, ptr);

            if(damage > 0.0f)
            {
                sndMgr->playSound3D(ptr, "Health Damage", 1.0f, 1.0f);
                if (ptr == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay();
            }
            MWMechanics::DynamicStat<float> health(getCreatureStats(ptr).getHealth());
            health.setCurrent(health.getCurrent() - damage);
            getCreatureStats(ptr).setHealth(health);
        }
        else
        {
            MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
            fatigue.setCurrent(fatigue.getCurrent() - damage, true);
            getCreatureStats(ptr).setFatigue(fatigue);
        }

        if (!wasDead && getCreatureStats(ptr).isDead())
        {
            // NPC was killed
            if (!attacker.isEmpty() && attacker.getClass().isNpc() && attacker.getClass().getNpcStats(attacker).isWerewolf())
            {
                attacker.getClass().getNpcStats(attacker).addWerewolfKill();
            }

            MWBase::Environment::get().getMechanicsManager()->actorKilled(ptr, attacker);
        }
    }

    boost::shared_ptr<MWWorld::Action> Npc::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        // player got activated by another NPC
        if(ptr == MWMechanics::getPlayer())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(actor));

        // Werewolfs can't activate NPCs
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfNPC");

            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        if(getCreatureStats(ptr).isDead())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr, true));
        if(ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::FailedAction("#{sActorInCombat}"));
        if(getCreatureStats(actor).getStance(MWMechanics::CreatureStats::Stance_Sneak)
                || ptr.getClass().getCreatureStats(ptr).getKnockedDown())
            return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr)); // stealing
        // Can't talk to werewolfs
        if(ptr.getClass().isNpc() && ptr.getClass().getNpcStats(ptr).isWerewolf())
            return boost::shared_ptr<MWWorld::Action> (new MWWorld::FailedAction(""));
        return boost::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));
    }

    MWWorld::ContainerStore& Npc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWWorld::InventoryStore& Npc::getInventoryStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    std::string Npc::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return ref->mBase->mScript;
    }

    float Npc::getSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const GMST& gmst = getGmst();

        const NpcCustomData *npcdata = static_cast<const NpcCustomData*>(ptr.getRefData().getCustomData());
        const MWMechanics::MagicEffects &mageffects = npcdata->mNpcStats.getMagicEffects();

        const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);

        bool sneaking = ptr.getClass().getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Sneak);
        bool running = ptr.getClass().getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run);

        float walkSpeed = gmst.fMinWalkSpeed->getFloat() + 0.01f*npcdata->mNpcStats.getAttribute(ESM::Attribute::Speed).getModified()*
                                                      (gmst.fMaxWalkSpeed->getFloat() - gmst.fMinWalkSpeed->getFloat());
        walkSpeed *= 1.0f - gmst.fEncumberedMoveEffect->getFloat()*normalizedEncumbrance;
        walkSpeed = std::max(0.0f, walkSpeed);
        if(sneaking)
            walkSpeed *= gmst.fSneakSpeedMultiplier->getFloat();

        float runSpeed = walkSpeed*(0.01f * npcdata->mNpcStats.getSkill(ESM::Skill::Athletics).getModified() *
                                    gmst.fAthleticsRunBonus->getFloat() + gmst.fBaseRunMultiplier->getFloat());

        float moveSpeed;
        if(getEncumbrance(ptr) > getCapacity(ptr))
            moveSpeed = 0.0f;
        else if(mageffects.get(ESM::MagicEffect::Levitate).getMagnitude() > 0 &&
                world->isLevitationEnabled())
        {
            float flySpeed = 0.01f*(npcdata->mNpcStats.getAttribute(ESM::Attribute::Speed).getModified() +
                                    mageffects.get(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->getFloat() + flySpeed*(gmst.fMaxFlySpeed->getFloat() - gmst.fMinFlySpeed->getFloat());
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if(world->isSwimming(ptr))
        {
            float swimSpeed = walkSpeed;
            if(running)
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(ESM::MagicEffect::SwiftSwim).getMagnitude();
            swimSpeed *= gmst.fSwimRunBase->getFloat() + 0.01f*npcdata->mNpcStats.getSkill(ESM::Skill::Athletics).getModified()*
                                                    gmst.fSwimRunAthleticsMult->getFloat();
            moveSpeed = swimSpeed;
        }
        else if(running && !sneaking)
            moveSpeed = runSpeed;
        else
            moveSpeed = walkSpeed;
        if(getMovementSettings(ptr).mPosition[0] != 0 && getMovementSettings(ptr).mPosition[1] == 0)
            moveSpeed *= 0.75f;

        if(npcdata->mNpcStats.isWerewolf() && running && npcdata->mNpcStats.getDrawState() == MWMechanics::DrawState_Nothing)
            moveSpeed *= gmst.fWereWolfRunMult->getFloat();

        return moveSpeed;
    }

    float Npc::getJump(const MWWorld::Ptr &ptr) const
    {
        if(getEncumbrance(ptr) > getCapacity(ptr))
            return 0.f;

        const NpcCustomData *npcdata = static_cast<const NpcCustomData*>(ptr.getRefData().getCustomData());
        const GMST& gmst = getGmst();
        const MWMechanics::MagicEffects &mageffects = npcdata->mNpcStats.getMagicEffects();
        const float encumbranceTerm = gmst.fJumpEncumbranceBase->getFloat() +
                                          gmst.fJumpEncumbranceMultiplier->getFloat() *
                                          (1.0f - Npc::getEncumbrance(ptr)/Npc::getCapacity(ptr));

        float a = static_cast<float>(npcdata->mNpcStats.getSkill(ESM::Skill::Acrobatics).getModified());
        float b = 0.0f;
        if(a > 50.0f)
        {
            b = a - 50.0f;
            a = 50.0f;
        }

        float x = gmst.fJumpAcrobaticsBase->getFloat() +
                  std::pow(a / 15.0f, gmst.fJumpAcroMultiplier->getFloat());
        x += 3.0f * b * gmst.fJumpAcroMultiplier->getFloat();
        x += mageffects.get(ESM::MagicEffect::Jump).getMagnitude() * 64;
        x *= encumbranceTerm;

        if(ptr.getClass().getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run))
            x *= gmst.fJumpRunMultiplier->getFloat();
        x *= npcdata->mNpcStats.getFatigueTerm();
        x -= -627.2f;/*gravity constant*/
        x /= 3.0f;

        return x;
    }

    MWMechanics::Movement& Npc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    bool Npc::isEssential (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return (ref->mBase->mFlags & ESM::NPC::Essential) != 0;
    }

    void Npc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Npc);
        registerClass (typeid (ESM::NPC).name(), instance);
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        info.caption = getName(ptr);
        if(fullHelp && getNpcStats(ptr).isWerewolf())
        {
            info.caption += " (";
            info.caption += ref->mBase->mName;
            info.caption += ")";
        }

        if(fullHelp)
            info.text = MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");

        return info;
    }

    float Npc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        static const float fEncumbranceStrMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fEncumbranceStrMult")->getFloat();
        return stats.getAttribute(0).getModified()*fEncumbranceStrMult;
    }

    float Npc::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        // According to UESP, inventory weight is ignored in werewolf form. Does that include
        // feather and burden effects?
        return getNpcStats(ptr).isWerewolf() ? 0.0f : Actor::getEncumbrance(ptr);
    }

    bool Npc::apply (const MWWorld::Ptr& ptr, const std::string& id,
        const MWWorld::Ptr& actor) const
    {
        MWMechanics::CastSpell cast(ptr, ptr);
        return cast.cast(id);
    }

    void Npc::skillUsageSucceeded (const MWWorld::Ptr& ptr, int skill, int usageType, float extraFactor) const
    {
        MWMechanics::NpcStats& stats = getNpcStats (ptr);

        if (stats.isWerewolf())
            return;

        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        const ESM::Class *class_ =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Class>().find (
                ref->mBase->mClass
            );

        stats.useSkill (skill, *class_, usageType, extraFactor);
    }

    float Npc::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

        MWMechanics::NpcStats &stats = getNpcStats(ptr);
        MWWorld::InventoryStore &invStore = getInventoryStore(ptr);

        float fUnarmoredBase1 = store.find("fUnarmoredBase1")->getFloat();
        float fUnarmoredBase2 = store.find("fUnarmoredBase2")->getFloat();
        int unarmoredSkill = stats.getSkill(ESM::Skill::Unarmored).getModified();

        int ratings[MWWorld::InventoryStore::Slots];
        for(int i = 0;i < MWWorld::InventoryStore::Slots;i++)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getTypeName() != typeid(ESM::Armor).name())
            {
                // unarmored
                ratings[i] = static_cast<int>((fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill));
            }
            else
            {
                ratings[i] = it->getClass().getEffectiveArmorRating(*it, ptr);
            }
        }

        float shield = stats.getMagicEffects().get(ESM::MagicEffect::Shield).getMagnitude();

        return ratings[MWWorld::InventoryStore::Slot_Cuirass] * 0.3f
                + (ratings[MWWorld::InventoryStore::Slot_CarriedLeft] + ratings[MWWorld::InventoryStore::Slot_Helmet]
                    + ratings[MWWorld::InventoryStore::Slot_Greaves] + ratings[MWWorld::InventoryStore::Slot_Boots]
                    + ratings[MWWorld::InventoryStore::Slot_LeftPauldron] + ratings[MWWorld::InventoryStore::Slot_RightPauldron]
                    ) * 0.1f
                + (ratings[MWWorld::InventoryStore::Slot_LeftGauntlet] + ratings[MWWorld::InventoryStore::Slot_RightGauntlet])
                    * 0.05f
                + shield;
    }

    void Npc::adjustScale(const MWWorld::Ptr &ptr, osg::Vec3f&scale, bool rendering) const
    {
        if (!rendering)
            return; // collision meshes are not scaled based on race height
                    // having the same collision extents for all races makes the environments easier to test

        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        const ESM::Race* race =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);

        if (ref->mBase->isMale())
        {
            scale.x() *= race->mData.mWeight.mMale;
            scale.y() *= race->mData.mWeight.mMale;
            scale.z() *= race->mData.mHeight.mMale;
        }
        else
        {
            scale.x() *= race->mData.mWeight.mFemale;
            scale.y() *= race->mData.mWeight.mFemale;
            scale.z() *= race->mData.mHeight.mFemale;
        }

    }

    int Npc::getServices(const MWWorld::Ptr &actor) const
    {
        MWWorld::LiveCellRef<ESM::NPC>* ref = actor.get<ESM::NPC>();
        if (ref->mBase->mHasAI)
            return ref->mBase->mAiData.mServices;
        else
            return 0;
    }


    std::string Npc::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        if(name == "left" || name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if(world->isSwimming(ptr))
                return (name == "left") ? "Swim Left" : "Swim Right";
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return (name == "left") ? "FootWaterLeft" : "FootWaterRight";
            if(world->isOnGround(ptr))
            {
                if (ptr.getClass().getNpcStats(ptr).isWerewolf()
                        && ptr.getClass().getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run))
                {
                    MWMechanics::WeaponType weaponType = MWMechanics::WeapType_None;
                    MWMechanics::getActiveWeapon(ptr.getClass().getCreatureStats(ptr), ptr.getClass().getInventoryStore(ptr), &weaponType);
                    if (weaponType == MWMechanics::WeapType_None)
                        return "";
                }

                MWWorld::InventoryStore &inv = Npc::getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
                if(boots == inv.end() || boots->getTypeName() != typeid(ESM::Armor).name())
                    return (name == "left") ? "FootBareLeft" : "FootBareRight";

                switch(boots->getClass().getEquipmentSkill(*boots))
                {
                    case ESM::Skill::LightArmor:
                        return (name == "left") ? "FootLightLeft" : "FootLightRight";
                    case ESM::Skill::MediumArmor:
                        return (name == "left") ? "FootMedLeft" : "FootMedRight";
                    case ESM::Skill::HeavyArmor:
                        return (name == "left") ? "FootHeavyLeft" : "FootHeavyRight";
                }
            }
            return "";
        }

        if(name == "land")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return "DefaultLandWater";
            if(world->isOnGround(ptr))
                return "Body Fall Medium";
            return "";
        }
        if(name == "swimleft")
            return "Swim Left";
        if(name == "swimright")
            return "Swim Right";
        // TODO: I have no idea what these are supposed to do for NPCs since they use
        // voiced dialog for various conditions like health loss and combat taunts. Maybe
        // only for biped creatures?

        if(name == "moan")
            return "";
        if(name == "roar")
            return "";
        if(name == "scream")
            return "";

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }

    MWWorld::Ptr
    Npc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM::NPC>();

        return MWWorld::Ptr(&cell.get<ESM::NPC>().insert(*ref), &cell);
    }

    int Npc::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        return ptr.getClass().getNpcStats(ptr).getSkill(skill).getModified();
    }

    int Npc::getBloodTexture(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        if (ref->mBase->mFlags & ESM::NPC::Skeleton)
            return 1;
        if (ref->mBase->mFlags & ESM::NPC::Metal)
            return 2;
        return 0;
    }

    void Npc::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::NpcState& state2 = dynamic_cast<const ESM::NpcState&> (state);

        if (state.mVersion > 0)
        {
            if (!ptr.getRefData().getCustomData())
            {
                // Create a CustomData, but don't fill it from ESM records (not needed)
                std::auto_ptr<NpcCustomData> data (new NpcCustomData);
                ptr.getRefData().setCustomData (data.release());
            }
        }
        else
            ensureCustomData(ptr); // in openmw 0.30 savegames not all state was saved yet, so need to load it regardless.

        NpcCustomData& customData = dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData());

        customData.mInventoryStore.readState (state2.mInventory);
        customData.mNpcStats.readState (state2.mNpcStats);
        static_cast<MWMechanics::CreatureStats&> (customData.mNpcStats).readState (state2.mCreatureStats);
    }

    void Npc::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
        const
    {
        ESM::NpcState& state2 = dynamic_cast<ESM::NpcState&> (state);

        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        ensureCustomData (ptr);

        NpcCustomData& customData = dynamic_cast<NpcCustomData&> (*ptr.getRefData().getCustomData());

        customData.mInventoryStore.writeState (state2.mInventory);
        customData.mNpcStats.writeState (state2.mNpcStats);
        static_cast<const MWMechanics::CreatureStats&> (customData.mNpcStats).writeState (state2.mCreatureStats);
    }

    int Npc::getBaseGold(const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        if(ref->mBase->mNpdtType != ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
            return ref->mBase->mNpdt52.mGold;
        else
            return ref->mBase->mNpdt12.mGold;
    }

    bool Npc::isClass(const MWWorld::Ptr& ptr, const std::string &className) const
    {
        return Misc::StringUtils::ciEqual(ptr.get<ESM::NPC>()->mBase->mClass, className);
    }

    void Npc::respawn(const MWWorld::Ptr &ptr) const
    {
        if (ptr.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Respawn)
        {
            // Note we do not respawn moved references in the cell they were moved to. Instead they are respawned in the original cell.
            // This also means we cannot respawn dynamically placed references with no content file connection.
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getRefData().getCount() == 0)
                    ptr.getRefData().setCount(1);

                // Reset to original position
                ptr.getRefData().setPosition(ptr.getCellRef().getPosition());

                ptr.getRefData().setCustomData(NULL);
            }
        }
    }

    void Npc::restock(const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        const ESM::InventoryList& list = ref->mBase->mInventory;
        MWWorld::ContainerStore& store = getContainerStore(ptr);
        store.restock(list, ptr, ptr.getCellRef().getRefId());
    }

    int Npc::getBaseFightRating (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mAiData.mFight;
    }

    bool Npc::isBipedal(const MWWorld::Ptr &ptr) const
    {
        return true;
    }

    std::string Npc::getPrimaryFaction (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mFaction;
    }

    int Npc::getPrimaryFactionRank (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->getFactionRank();
    }
}
