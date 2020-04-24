#include "npc.hpp"

#include <memory>

#include <components/misc/constants.hpp>
#include <components/misc/rng.hpp>

#include <components/debug/debuglog.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm/npcstate.hpp>
#include <components/settings/settings.hpp>

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
#include "../mwmechanics/weapontype.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/actionopen.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwphysics/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/localscripts.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"
#include "../mwrender/npcanimation.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{

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

    class NpcCustomData : public MWWorld::CustomData
    {
    public:
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;

        virtual NpcCustomData& asNpcCustomData()
        {
            return *this;
        }
        virtual const NpcCustomData& asNpcCustomData() const
        {
            return *this;
        }
    };

    MWWorld::CustomData *NpcCustomData::clone() const
    {
        return new NpcCustomData (*this);
    }

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
            std::unique_ptr<NpcCustomData> data(new NpcCustomData);

            MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

            // creature stats
            int gold=0;
            if(ref->mBase->mNpdtType != ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
            {
                gold = ref->mBase->mNpdt.mGold;

                for (unsigned int i=0; i< ESM::Skill::Length; ++i)
                    data->mNpcStats.getSkill (i).setBase (ref->mBase->mNpdt.mSkills[i]);

                data->mNpcStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mNpdt.mStrength);
                data->mNpcStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mNpdt.mIntelligence);
                data->mNpcStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mNpdt.mWillpower);
                data->mNpcStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mNpdt.mAgility);
                data->mNpcStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mNpdt.mSpeed);
                data->mNpcStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mNpdt.mEndurance);
                data->mNpcStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mNpdt.mPersonality);
                data->mNpcStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mNpdt.mLuck);

                data->mNpcStats.setHealth (ref->mBase->mNpdt.mHealth);
                data->mNpcStats.setMagicka (ref->mBase->mNpdt.mMana);
                data->mNpcStats.setFatigue (ref->mBase->mNpdt.mFatigue);

                data->mNpcStats.setLevel(ref->mBase->mNpdt.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt.mReputation);

                data->mNpcStats.setNeedRecalcDynamicStats(false);
            }
            else
            {
                gold = ref->mBase->mNpdt.mGold;

                for (int i=0; i<3; ++i)
                    data->mNpcStats.setDynamic (i, 10);

                data->mNpcStats.setLevel(ref->mBase->mNpdt.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt.mReputation);

                autoCalculateAttributes(ref->mBase, data->mNpcStats);
                autoCalculateSkills(ref->mBase, data->mNpcStats, ptr);

                data->mNpcStats.setNeedRecalcDynamicStats(true);
            }

            // Persistent actors with 0 health do not play death animation
            if (data->mNpcStats.isDead())
                data->mNpcStats.setDeathAnimationFinished(isPersistent(ptr));

            // race powers
            const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
            for (std::vector<std::string>::const_iterator iter (race->mPowers.mList.begin());
                iter!=race->mPowers.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                    Log(Debug::Warning) << "Warning: ignoring nonexistent race power '" << *iter << "' on NPC '" << ref->mBase->mId << "'";
            }

            if (!ref->mBase->mFaction.empty())
            {
                static const int iAutoRepFacMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepFacMod")->mValue.getInteger();
                static const int iAutoRepLevMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepLevMod")->mValue.getInteger();
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
                    Log(Debug::Warning) << "Warning: ignoring nonexistent spell '" << *iter << "' on NPC '" << ref->mBase->mId << "'";
                }
            }

            // inventory
            // setting ownership is used to make the NPC auto-equip his initial equipment only, and not bartered items
            data->mInventoryStore.fill(ref->mBase->mInventory, ptr.getCellRef().getRefId());

            data->mNpcStats.setGoldPool(gold);

            // store
            ptr.getRefData().setCustomData (data.release());

            getInventoryStore(ptr).autoEquip(ptr);
        }
    }

    void Npc::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getObjects().insertNPC(ptr);
    }

    bool Npc::isPersistent(const MWWorld::ConstPtr &actor) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = actor.get<ESM::NPC>();
        return ref->mBase->mPersistent;
    }

    std::string Npc::getModel(const MWWorld::ConstPtr &ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        std::string model = "meshes\\base_anim.nif";
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
        if(race->mData.mFlags & ESM::Race::Beast)
            model = "meshes\\base_animkna.nif";

        return model;
    }

    void Npc::getModelsToPreload(const MWWorld::Ptr &ptr, std::vector<std::string> &models) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *npc = ptr.get<ESM::NPC>();
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().search(npc->mBase->mRace);
        if(race && race->mData.mFlags & ESM::Race::Beast)
            models.push_back("meshes\\base_animkna.nif");

        // keep these always loaded just in case
        models.push_back("meshes/xargonian_swimkna.nif");
        models.push_back("meshes/xbase_anim_female.nif");
        models.push_back("meshes/xbase_anim.nif");

        if (!npc->mBase->mModel.empty())
            models.push_back("meshes/"+npc->mBase->mModel);

        if (!npc->mBase->mHead.empty())
        {
            const ESM::BodyPart* head = MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>().search(npc->mBase->mHead);
            if (head)
                models.push_back("meshes/"+head->mModel);
        }
        if (!npc->mBase->mHair.empty())
        {
            const ESM::BodyPart* hair = MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>().search(npc->mBase->mHair);
            if (hair)
                models.push_back("meshes/"+hair->mModel);
        }

        bool female = (npc->mBase->mFlags & ESM::NPC::Female);

        // FIXME: use const version of InventoryStore functions once they are available
        // preload equipped items
        const MWWorld::InventoryStore& invStore = getInventoryStore(ptr);
        for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            MWWorld::ConstContainerStoreIterator equipped = invStore.getSlot(slot);
            if (equipped != invStore.end())
            {
                std::vector<ESM::PartReference> parts;
                if(equipped->getTypeName() == typeid(ESM::Clothing).name())
                {
                    const ESM::Clothing *clothes = equipped->get<ESM::Clothing>()->mBase;
                    parts = clothes->mParts.mParts;
                }
                else if(equipped->getTypeName() == typeid(ESM::Armor).name())
                {
                    const ESM::Armor *armor = equipped->get<ESM::Armor>()->mBase;
                    parts = armor->mParts.mParts;
                }
                else
                {
                    std::string model = equipped->getClass().getModel(*equipped);
                    if (!model.empty())
                        models.push_back(model);
                }

                for (std::vector<ESM::PartReference>::const_iterator it = parts.begin(); it != parts.end(); ++it)
                {
                    std::string partname = female ? it->mFemale : it->mMale;
                    if (partname.empty())
                        partname = female ? it->mMale : it->mFemale;
                    const ESM::BodyPart* part = MWBase::Environment::get().getWorld()->getStore().get<ESM::BodyPart>().search(partname);
                    if (part && !part->mModel.empty())
                        models.push_back("meshes/"+part->mModel);
                }
            }
        }

        // preload body parts
        if (race)
        {
            const std::vector<const ESM::BodyPart*>& parts = MWRender::NpcAnimation::getBodyParts(Misc::StringUtils::lowerCase(race->mId), female, false, false);
            for (std::vector<const ESM::BodyPart*>::const_iterator it = parts.begin(); it != parts.end(); ++it)
            {
                const ESM::BodyPart* part = *it;
                if (part && !part->mModel.empty())
                    models.push_back("meshes/"+part->mModel);
            }
        }

    }

    std::string Npc::getName (const MWWorld::ConstPtr& ptr) const
    {
        if(ptr.getRefData().getCustomData() && ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats.isWerewolf())
        {
            const MWBase::World *world = MWBase::Environment::get().getWorld();
            const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

            return store.find("sWerewolfPopup")->mValue.getString();
        }

        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId;
    }

    MWMechanics::CreatureStats& Npc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats;
    }

    MWMechanics::NpcStats& Npc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats;
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

        const float fCombatDistance = store.find("fCombatDistance")->mValue.getFloat();
        float dist = fCombatDistance * (!weapon.isEmpty() ?
                               weapon.get<ESM::Weapon>()->mBase->mData.mReach :
                               store.find("fHandToHandReach")->mValue.getFloat());

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit result.
        std::vector<MWWorld::Ptr> targetActors;
        if (ptr != MWMechanics::getPlayer())
            getCreatureStats(ptr).getAiSequence().getCombatTargets(targetActors);

        // TODO: Use second to work out the hit angle
        std::pair<MWWorld::Ptr, osg::Vec3f> result = world->getHitContact(ptr, dist, targetActors);
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

        float hitchance = MWMechanics::getHitChance(ptr, victim, getSkill(ptr, weapskill));

        if (Misc::Rng::roll0to99() >= hitchance)
        {
            othercls.onHit(victim, 0.0f, false, weapon, ptr, osg::Vec3f(), false);
            MWMechanics::reduceWeaponCondition(0.f, false, weapon, ptr);
            return;
        }

        bool healthdmg;
        float damage = 0.0f;
        if(!weapon.isEmpty())
        {
            const unsigned char *attack = nullptr;
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
            MWMechanics::resistNormalWeapon(victim, ptr, weapon, damage);
            MWMechanics::applyWerewolfDamageMult(victim, weapon, damage);
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
                damage *= store.find("fCombatCriticalStrikeMult")->mValue.getFloat();
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTargetCriticalStrike}");
                MWBase::Environment::get().getSoundManager()->playSound3D(victim, "critical damage", 1.0f, 1.0f);
            }
        }

        if (othercls.getCreatureStats(victim).getKnockedDown())
            damage *= store.find("fCombatKODamageMult")->mValue.getFloat();

        // Apply "On hit" enchanted weapons
        MWMechanics::applyOnStrikeEnchantment(ptr, victim, weapon, hitPosition);

        MWMechanics::applyElementalShields(ptr, victim);

        if (MWMechanics::blockMeleeAttack(ptr, victim, weapon, damage, attackStrength))
            damage = 0;

        if (victim == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState())
            damage = 0;

        MWMechanics::diseaseContact(victim, ptr);

        othercls.onHit(victim, damage, healthdmg, weapon, ptr, hitPosition, true);
    }

    void Npc::onHit(const MWWorld::Ptr &ptr, float damage, bool ishealth, const MWWorld::Ptr &object, const MWWorld::Ptr &attacker, const osg::Vec3f &hitPosition, bool successful) const
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        bool wasDead = stats.isDead();

        // Note OnPcHitMe is not set for friendly hits.
        bool setOnPcHitMe = true;

        // NOTE: 'object' and/or 'attacker' may be empty.
        if (!attacker.isEmpty() && attacker.getClass().isActor() && !stats.getAiSequence().isInCombat(attacker))
        {
            stats.setAttacked(true);
            setOnPcHitMe = MWBase::Environment::get().getMechanicsManager()->actorAttacked(ptr, attacker);
        }

        // Attacker and target store each other as hitattemptactor if they have no one stored yet
        if (!attacker.isEmpty() && attacker.getClass().isActor())
        {
            MWMechanics::CreatureStats& statsAttacker = attacker.getClass().getCreatureStats(attacker);
            // First handle the attacked actor
            if ((stats.getHitAttemptActorId() == -1)
                && (statsAttacker.getAiSequence().isInCombat(ptr)
                    || attacker == MWMechanics::getPlayer()))
                stats.setHitAttemptActorId(statsAttacker.getActorId());

            // Next handle the attacking actor
            if ((statsAttacker.getHitAttemptActorId() == -1)
                && (statsAttacker.getAiSequence().isInCombat(ptr)
                    || attacker == MWMechanics::getPlayer()))
                statsAttacker.setHitAttemptActorId(stats.getActorId());
        }

        if (!object.isEmpty())
            stats.setLastHitAttemptObject(object.getCellRef().getRefId());

        if (setOnPcHitMe && !attacker.isEmpty() && attacker == MWMechanics::getPlayer())
        {
            const std::string &script = getScript(ptr);
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if(!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if (!successful)
        {
            // Missed
            if (!attacker.isEmpty() && attacker == MWMechanics::getPlayer())
                sndMgr->playSound3D(ptr, "miss", 1.0f, 1.0f);
            return;
        }

        if (!object.isEmpty())
            stats.setLastHitObject(object.getCellRef().getRefId());


        if (damage > 0.0f && !object.isEmpty())
            MWMechanics::resistNormalWeapon(ptr, attacker, object, damage);

        if (damage < 0.001f)
            damage = 0;

        bool godmode = ptr == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        if (godmode)
            damage = 0;

        if (damage > 0.0f && !attacker.isEmpty())
        {
            // 'ptr' is losing health. Play a 'hit' voiced dialog entry if not already saying
            // something, alert the character controller, scripts, etc.

            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const GMST& gmst = getGmst();

            int chance = store.get<ESM::GameSetting>().find("iVoiceHitOdds")->mValue.getInteger();
            if (Misc::Rng::roll0to99() < chance)
                MWBase::Environment::get().getDialogueManager()->say(ptr, "hit");

            // Check for knockdown
            float agilityTerm = stats.getAttribute(ESM::Attribute::Agility).getModified() * gmst.fKnockDownMult->mValue.getFloat();
            float knockdownTerm = stats.getAttribute(ESM::Attribute::Agility).getModified()
                    * gmst.iKnockDownOddsMult->mValue.getInteger() * 0.01f + gmst.iKnockDownOddsBase->mValue.getInteger();
            if (ishealth && agilityTerm <= damage && knockdownTerm <= Misc::Rng::roll0to99())
                stats.setKnockedDown(true);
            else
                stats.setHitRecovery(true); // Is this supposed to always occur?

            if (damage > 0 && ishealth)
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
                damage *= std::max(gmst.fCombatArmorMinMult->mValue.getFloat(), x);
                int damageDiff = static_cast<int>(unmitigatedDamage - damage);
                damage = std::max(1.f, damage);
                damageDiff = std::max(1, damageDiff);

                MWWorld::InventoryStore &inv = getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator armorslot = inv.getSlot(hitslot);
                MWWorld::Ptr armor = ((armorslot != inv.end()) ? *armorslot : MWWorld::Ptr());
                if(!armor.isEmpty() && armor.getTypeName() == typeid(ESM::Armor).name())
                {
                    if (!object.isEmpty() || attacker.isEmpty() || attacker.getClass().isNpc()) // Unarmed creature attacks don't affect armor condition
                    {
                        int armorhealth = armor.getClass().getItemHealth(armor);
                        armorhealth -= std::min(damageDiff, armorhealth);
                        armor.getCellRef().setCharge(armorhealth);

                        // Armor broken? unequip it
                        if (armorhealth == 0)
                            armor = *inv.unequipItem(armor, ptr);
                    }

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

        if (ishealth)
        {
            if (!attacker.isEmpty() && !godmode)
                damage = scaleDamage(damage, attacker, ptr);

            if (damage > 0.0f)
            {
                sndMgr->playSound3D(ptr, "Health Damage", 1.0f, 1.0f);
                if (ptr == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->activateHitOverlay();
                if (!attacker.isEmpty())
                    MWBase::Environment::get().getWorld()->spawnBloodEffect(ptr, hitPosition);
            }
            MWMechanics::DynamicStat<float> health(getCreatureStats(ptr).getHealth());
            health.setCurrent(health.getCurrent() - damage);
            stats.setHealth(health);
        }
        else
        {
            MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
            fatigue.setCurrent(fatigue.getCurrent() - damage, true);
            stats.setFatigue(fatigue);
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

    std::shared_ptr<MWWorld::Action> Npc::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        // player got activated by another NPC
        if(ptr == MWMechanics::getPlayer())
            return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(actor));

        // Werewolfs can't activate NPCs
        if(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfNPC");

            std::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction("#{sWerewolfRefusal}"));
            if(sound) action->setSound(sound->mId);

            return action;
        }

        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        if(stats.isDead())
        {
            bool canLoot = Settings::Manager::getBool ("can loot during death animation", "Game");

            // by default user can loot friendly actors during death animation
            if (canLoot && !stats.getAiSequence().isInCombat())
                return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr));

            // otherwise wait until death animation
            if(stats.isDeathAnimationFinished())
                return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr));
        }
        else if (!stats.getAiSequence().isInCombat())
        {
            if(getCreatureStats(actor).getStance(MWMechanics::CreatureStats::Stance_Sneak) || stats.getKnockedDown())
                return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr)); // stealing

            // Can't talk to werewolves
            if (!getNpcStats(ptr).isWerewolf())
                return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionTalk(ptr));
        }

        // Tribunal and some mod companions oddly enough must use open action as fallback
        if (!getScript(ptr).empty() && ptr.getRefData().getLocals().getIntVar(getScript(ptr), "companion"))
            return std::shared_ptr<MWWorld::Action>(new MWWorld::ActionOpen(ptr));

        return std::shared_ptr<MWWorld::Action> (new MWWorld::FailedAction(""));
    }

    MWWorld::ContainerStore& Npc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mInventoryStore;
    }

    MWWorld::InventoryStore& Npc::getInventoryStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mInventoryStore;
    }

    std::string Npc::getScript (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        return ref->mBase->mScript;
    }

    float Npc::getSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const GMST& gmst = getGmst();

        const NpcCustomData *npcdata = static_cast<const NpcCustomData*>(ptr.getRefData().getCustomData());
        const MWMechanics::MagicEffects &mageffects = npcdata->mNpcStats.getMagicEffects();

        const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);

        bool swimming = world->isSwimming(ptr);
        bool sneaking = MWBase::Environment::get().getMechanicsManager()->isSneaking(ptr);
        bool running = stats.getStance(MWMechanics::CreatureStats::Stance_Run);
        bool inair = !world->isOnGround(ptr) && !swimming && !world->isFlying(ptr);
        running = running && (inair || MWBase::Environment::get().getMechanicsManager()->isRunning(ptr));

        float walkSpeed = gmst.fMinWalkSpeed->mValue.getFloat() + 0.01f*npcdata->mNpcStats.getAttribute(ESM::Attribute::Speed).getModified()*
                                                      (gmst.fMaxWalkSpeed->mValue.getFloat() - gmst.fMinWalkSpeed->mValue.getFloat());
        walkSpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat()*normalizedEncumbrance;
        walkSpeed = std::max(0.0f, walkSpeed);
        if(sneaking)
            walkSpeed *= gmst.fSneakSpeedMultiplier->mValue.getFloat();

        float runSpeed = walkSpeed*(0.01f * getSkill(ptr, ESM::Skill::Athletics) *
                                    gmst.fAthleticsRunBonus->mValue.getFloat() + gmst.fBaseRunMultiplier->mValue.getFloat());

        float moveSpeed;
        if(getEncumbrance(ptr) > getCapacity(ptr))
            moveSpeed = 0.0f;
        else if(mageffects.get(ESM::MagicEffect::Levitate).getMagnitude() > 0 &&
                world->isLevitationEnabled())
        {
            float flySpeed = 0.01f*(npcdata->mNpcStats.getAttribute(ESM::Attribute::Speed).getModified() +
                                    mageffects.get(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->mValue.getFloat() + flySpeed*(gmst.fMaxFlySpeed->mValue.getFloat() - gmst.fMinFlySpeed->mValue.getFloat());
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if (swimming)
        {
            float swimSpeed = walkSpeed;
            if(running)
                swimSpeed = runSpeed;
            swimSpeed *= 1.0f + 0.01f * mageffects.get(ESM::MagicEffect::SwiftSwim).getMagnitude();
            swimSpeed *= gmst.fSwimRunBase->mValue.getFloat() + 0.01f*getSkill(ptr, ESM::Skill::Athletics)*
                                                    gmst.fSwimRunAthleticsMult->mValue.getFloat();
            moveSpeed = swimSpeed;
        }
        else if (running && !sneaking)
            moveSpeed = runSpeed;
        else
            moveSpeed = walkSpeed;
        if(getMovementSettings(ptr).mPosition[0] != 0 && getMovementSettings(ptr).mPosition[1] == 0)
            moveSpeed *= 0.75f;

        if(npcdata->mNpcStats.isWerewolf() && running && npcdata->mNpcStats.getDrawState() == MWMechanics::DrawState_Nothing)
            moveSpeed *= gmst.fWereWolfRunMult->mValue.getFloat();

        moveSpeed *= ptr.getClass().getMovementSettings(ptr).mSpeedFactor;

        return moveSpeed;
    }

    float Npc::getJump(const MWWorld::Ptr &ptr) const
    {
        if(getEncumbrance(ptr) > getCapacity(ptr))
            return 0.f;

        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const NpcCustomData *npcdata = static_cast<const NpcCustomData*>(ptr.getRefData().getCustomData());
        const GMST& gmst = getGmst();
        const MWMechanics::MagicEffects &mageffects = npcdata->mNpcStats.getMagicEffects();
        const float encumbranceTerm = gmst.fJumpEncumbranceBase->mValue.getFloat() +
                                          gmst.fJumpEncumbranceMultiplier->mValue.getFloat() *
                                          (1.0f - Npc::getNormalizedEncumbrance(ptr));

        float a = static_cast<float>(getSkill(ptr, ESM::Skill::Acrobatics));
        float b = 0.0f;
        if(a > 50.0f)
        {
            b = a - 50.0f;
            a = 50.0f;
        }

        float x = gmst.fJumpAcrobaticsBase->mValue.getFloat() +
                  std::pow(a / 15.0f, gmst.fJumpAcroMultiplier->mValue.getFloat());
        x += 3.0f * b * gmst.fJumpAcroMultiplier->mValue.getFloat();
        x += mageffects.get(ESM::MagicEffect::Jump).getMagnitude() * 64;
        x *= encumbranceTerm;

        if(stats.getStance(MWMechanics::CreatureStats::Stance_Run))
            x *= gmst.fJumpRunMultiplier->mValue.getFloat();
        x *= npcdata->mNpcStats.getFatigueTerm();
        x -= -Constants::GravityConst * Constants::UnitsPerMeter;
        x /= 3.0f;

        return x;
    }

    MWMechanics::Movement& Npc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mMovement;
    }

    bool Npc::isEssential (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        return (ref->mBase->mFlags & ESM::NPC::Essential) != 0;
    }

    void Npc::registerSelf()
    {
        std::shared_ptr<Class> instance (new Npc);
        registerClass (typeid (ESM::NPC).name(), instance);
    }

    bool Npc::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        if (!ptr.getRefData().getCustomData() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return true;

        const NpcCustomData& customData = ptr.getRefData().getCustomData()->asNpcCustomData();

        if (customData.mNpcStats.isDead() && customData.mNpcStats.isDeathAnimationFinished())
            return true;

        return !customData.mNpcStats.getAiSequence().isInCombat();
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo (const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        info.caption = MyGUI::TextIterator::toTagsString(getName(ptr));
        if(fullHelp && !ref->mBase->mName.empty() && ptr.getRefData().getCustomData() && ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats.isWerewolf())
        {
            info.caption += " (";
            info.caption += MyGUI::TextIterator::toTagsString(ref->mBase->mName);
            info.caption += ")";
        }

        if(fullHelp)
            info.text = MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");

        return info;
    }

    float Npc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        static const float fEncumbranceStrMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fEncumbranceStrMult")->mValue.getFloat();
        return stats.getAttribute(ESM::Attribute::Strength).getModified()*fEncumbranceStrMult;
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
        const MWWorld::InventoryStore &invStore = getInventoryStore(ptr);

        float fUnarmoredBase1 = store.find("fUnarmoredBase1")->mValue.getFloat();
        float fUnarmoredBase2 = store.find("fUnarmoredBase2")->mValue.getFloat();
        int unarmoredSkill = getSkill(ptr, ESM::Skill::Unarmored);

        float ratings[MWWorld::InventoryStore::Slots];
        for(int i = 0;i < MWWorld::InventoryStore::Slots;i++)
        {
            MWWorld::ConstContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getTypeName() != typeid(ESM::Armor).name())
            {
                // unarmored
                ratings[i] = (fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill);
            }
            else
            {
                ratings[i] = it->getClass().getEffectiveArmorRating(*it, ptr);

                // Take in account armor condition
                const bool hasHealth = it->getClass().hasItemHealth(*it);
                if (hasHealth)
                {
                    ratings[i] *= it->getClass().getItemNormalizedHealth(*it);
                }
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

    void Npc::adjustScale(const MWWorld::ConstPtr &ptr, osg::Vec3f&scale, bool rendering) const
    {
        if (!rendering)
            return; // collision meshes are not scaled based on race height
                    // having the same collision extents for all races makes the environments easier to test

        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        const ESM::Race* race =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);

        // Race weight should not affect 1st-person meshes, otherwise it will change hand proportions and can break aiming.
        if (ptr == MWMechanics::getPlayer() && ptr.isInCell() && MWBase::Environment::get().getWorld()->isFirstPerson())
        {
            if (ref->mBase->isMale())
                scale *= race->mData.mHeight.mMale;
            else
                scale *= race->mData.mHeight.mFemale;

            return;
        }

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

    int Npc::getServices(const MWWorld::ConstPtr &actor) const
    {
        return actor.get<ESM::NPC>()->mBase->mAiData.mServices;
    }


    std::string Npc::getSoundIdFromSndGen(const MWWorld::Ptr &ptr, const std::string &name) const
    {
        if(name == "left" || name == "right")
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            if(world->isFlying(ptr))
                return std::string();
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if(world->isSwimming(ptr))
                return (name == "left") ? "Swim Left" : "Swim Right";
            if(world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return (name == "left") ? "FootWaterLeft" : "FootWaterRight";
            if(world->isOnGround(ptr))
            {
                if (getNpcStats(ptr).isWerewolf()
                        && getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run))
                {
                    int weaponType = ESM::Weapon::None;
                    MWMechanics::getActiveWeapon(ptr, &weaponType);
                    if (weaponType == ESM::Weapon::None)
                        return std::string();
                }

                const MWWorld::InventoryStore &inv = Npc::getInventoryStore(ptr);
                MWWorld::ConstContainerStoreIterator boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
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
            return std::string();
        }

        // Morrowind ignores land soundgen for NPCs
        if(name == "land")
            return std::string();
        if(name == "swimleft")
            return "Swim Left";
        if(name == "swimright")
            return "Swim Right";
        // TODO: I have no idea what these are supposed to do for NPCs since they use
        // voiced dialog for various conditions like health loss and combat taunts. Maybe
        // only for biped creatures?

        if(name == "moan")
            return std::string();
        if(name == "roar")
            return std::string();
        if(name == "scream")
            return std::string();

        throw std::runtime_error(std::string("Unexpected soundgen type: ")+name);
    }

    MWWorld::Ptr Npc::copyToCellImpl(const MWWorld::ConstPtr &ptr, MWWorld::CellStore &cell) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    int Npc::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        return getNpcStats(ptr).getSkill(skill).getModified();
    }

    int Npc::getBloodTexture(const MWWorld::ConstPtr &ptr) const
    {
        return ptr.get<ESM::NPC>()->mBase->mBloodType;
    }

    void Npc::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
        if (!state.mHasCustomState)
            return;

        if (state.mVersion > 0)
        {
            if (!ptr.getRefData().getCustomData())
            {
                // Create a CustomData, but don't fill it from ESM records (not needed)
                std::unique_ptr<NpcCustomData> data (new NpcCustomData);
                ptr.getRefData().setCustomData (data.release());
            }
        }
        else
            ensureCustomData(ptr); // in openmw 0.30 savegames not all state was saved yet, so need to load it regardless.

        NpcCustomData& customData = ptr.getRefData().getCustomData()->asNpcCustomData();
        const ESM::NpcState& npcState = state.asNpcState();
        customData.mInventoryStore.readState (npcState.mInventory);
        customData.mNpcStats.readState (npcState.mNpcStats);
        customData.mNpcStats.readState (npcState.mCreatureStats);
    }

    void Npc::writeAdditionalState (const MWWorld::ConstPtr& ptr, ESM::ObjectState& state)
        const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const NpcCustomData& customData = ptr.getRefData().getCustomData()->asNpcCustomData();
        ESM::NpcState& npcState = state.asNpcState();
        customData.mInventoryStore.writeState (npcState.mInventory);
        customData.mNpcStats.writeState (npcState.mNpcStats);
        customData.mNpcStats.writeState (npcState.mCreatureStats);
    }

    int Npc::getBaseGold(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mNpdt.mGold;
    }

    bool Npc::isClass(const MWWorld::ConstPtr& ptr, const std::string &className) const
    {
        return Misc::StringUtils::ciEqual(ptr.get<ESM::NPC>()->mBase->mClass, className);
    }

    bool Npc::canSwim(const MWWorld::ConstPtr &ptr) const
    {
        return true;
    }

    bool Npc::canWalk(const MWWorld::ConstPtr &ptr) const
    {
        return true;
    }

    void Npc::respawn(const MWWorld::Ptr &ptr) const
    {
        const MWMechanics::CreatureStats& creatureStats = getCreatureStats(ptr);
        if (ptr.getRefData().getCount() > 0 && !creatureStats.isDead())
            return;

        if (!creatureStats.isDeathAnimationFinished())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
        static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

        float delay = ptr.getRefData().getCount() == 0 ? fCorpseClearDelay : std::min(fCorpseRespawnDelay, fCorpseClearDelay);

        if (ptr.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Respawn
                && creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getRefData().getCount() == 0)
                {
                    ptr.getRefData().setCount(1);
                    const std::string& script = getScript(ptr);
                    if (!script.empty())
                        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, ptr);
                }

                MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                ptr.getRefData().setCustomData(nullptr);

                // Reset to original position
                MWBase::Environment::get().getWorld()->moveObject(ptr, ptr.getCellRef().getPosition().pos[0],
                        ptr.getCellRef().getPosition().pos[1],
                        ptr.getCellRef().getPosition().pos[2]);
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

    int Npc::getBaseFightRating (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mAiData.mFight;
    }

    bool Npc::isBipedal(const MWWorld::ConstPtr &ptr) const
    {
        return true;
    }

    std::string Npc::getPrimaryFaction (const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->mFaction;
    }

    int Npc::getPrimaryFactionRank (const MWWorld::ConstPtr& ptr) const
    {
        std::string factionID = ptr.getClass().getPrimaryFaction(ptr);
        if(factionID.empty())
            return -1;

        // Search in the NPC data first
        if (const MWWorld::CustomData* data = ptr.getRefData().getCustomData())
        {
            int rank = data->asNpcCustomData().mNpcStats.getFactionRank(factionID);
            if (rank >= 0)
                return rank;
        }

        // Use base NPC record as a fallback
        const MWWorld::LiveCellRef<ESM::NPC> *ref = ptr.get<ESM::NPC>();
        return ref->mBase->getFactionRank();
    }
}
