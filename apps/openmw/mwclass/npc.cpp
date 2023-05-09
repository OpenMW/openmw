﻿#include "npc.hpp"

#include <MyGUI_TextIterator.h>

#include <memory>

#include <components/misc/constants.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/esm3/npcstate.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aisetting.hpp"
#include "../mwmechanics/autocalcspell.hpp"
#include "../mwmechanics/combat.hpp"
#include "../mwmechanics/creaturecustomdataresetter.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/difficultyscaling.hpp"
#include "../mwmechanics/disease.hpp"
#include "../mwmechanics/inventory.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/setbaseaisetting.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "../mwworld/actionopen.hpp"
#include "../mwworld/actiontalk.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/failedaction.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/localscripts.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwrender/npcanimation.hpp"
#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"
#include "../mwgui/ustring.hpp"

namespace
{

    int is_even(double d)
    {
        double int_part;
        modf(d / 2.0, &int_part);
        return 2.0 * int_part == d;
    }

    int round_ieee_754(double d)
    {
        double i = floor(d);
        d -= i;
        if (d < 0.5)
            return static_cast<int>(i);
        if (d > 0.5)
            return static_cast<int>(i) + 1;
        if (is_even(i))
            return static_cast<int>(i);
        return static_cast<int>(i) + 1;
    }

    void autoCalculateAttributes(const ESM::NPC* npc, MWMechanics::CreatureStats& creatureStats)
    {
        // race bonus
        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(npc->mRace);

        bool male = (npc->mFlags & ESM::NPC::Female) == 0;

        int level = creatureStats.getLevel();
        for (int i = 0; i < ESM::Attribute::Length; ++i)
        {
            const ESM::Race::MaleFemale& attribute = race->mData.mAttributeValues[i];
            creatureStats.setAttribute(i, male ? attribute.mMale : attribute.mFemale);
        }

        // class bonus
        const ESM::Class* class_ = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);

        for (int i = 0; i < 2; ++i)
        {
            int attribute = class_->mData.mAttribute[i];
            if (attribute >= 0 && attribute < 8)
            {
                creatureStats.setAttribute(attribute, creatureStats.getAttribute(attribute).getBase() + 10);
            }
        }

        // skill bonus
        for (int attribute = 0; attribute < ESM::Attribute::Length; ++attribute)
        {
            float modifierSum = 0;

            for (int j = 0; j < ESM::Skill::Length; ++j)
            {
                const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(j);

                if (skill->mData.mAttribute != attribute)
                    continue;

                // is this a minor or major skill?
                float add = 0.2f;
                for (int k = 0; k < 5; ++k)
                {
                    if (class_->mData.mSkills[k][0] == j)
                        add = 0.5;
                }
                for (int k = 0; k < 5; ++k)
                {
                    if (class_->mData.mSkills[k][1] == j)
                        add = 1.0;
                }
                modifierSum += add;
            }
            creatureStats.setAttribute(attribute,
                std::min(
                    round_ieee_754(creatureStats.getAttribute(attribute).getBase() + (level - 1) * modifierSum), 100));
        }

        // initial health
        float strength = creatureStats.getAttribute(ESM::Attribute::Strength).getBase();
        float endurance = creatureStats.getAttribute(ESM::Attribute::Endurance).getBase();

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
    void autoCalculateSkills(
        const ESM::NPC* npc, MWMechanics::NpcStats& npcStats, const MWWorld::Ptr& ptr, bool spellsInitialised)
    {
        const ESM::Class* class_ = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);

        unsigned int level = npcStats.getLevel();

        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(npc->mRace);

        for (int i = 0; i < 2; ++i)
        {
            int bonus = (i == 0) ? 10 : 25;

            for (int i2 = 0; i2 < 5; ++i2)
            {
                int index = class_->mData.mSkills[i2][i];
                if (index >= 0 && index < ESM::Skill::Length)
                {
                    npcStats.getSkill(index).setBase(npcStats.getSkill(index).getBase() + bonus);
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
            const ESM::Skill* skill = MWBase::Environment::get().getESMStore()->get<ESM::Skill>().find(skillIndex);
            if (skill->mData.mSpecialization == class_->mData.mSpecialization)
            {
                specMultiplier = 0.5f;
                specBonus = 5;
            }

            npcStats.getSkill(skillIndex)
                .setBase(std::min(round_ieee_754(npcStats.getSkill(skillIndex).getBase() + 5 + raceBonus + specBonus
                                      + (int(level) - 1) * (majorMultiplier + specMultiplier)),
                    100)); // Must gracefully handle level 0
        }

        int skills[ESM::Skill::Length];
        for (int i = 0; i < ESM::Skill::Length; ++i)
            skills[i] = npcStats.getSkill(i).getBase();

        int attributes[ESM::Attribute::Length];
        for (int i = 0; i < ESM::Attribute::Length; ++i)
            attributes[i] = npcStats.getAttribute(i).getBase();

        if (!spellsInitialised)
        {
            std::vector<ESM::RefId> spells = MWMechanics::autoCalcNpcSpells(skills, attributes, race);
            npcStats.getSpells().addAllToInstance(spells);
        }
    }
}

namespace MWClass
{
    Npc::Npc()
        : MWWorld::RegisteredClass<Npc, Actor>(ESM::NPC::sRecordId)
    {
    }

    class NpcCustomData : public MWWorld::TypedCustomData<NpcCustomData>
    {
    public:
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        NpcCustomData& asNpcCustomData() override { return *this; }
        const NpcCustomData& asNpcCustomData() const override { return *this; }
    };

    const Npc::GMST& Npc::getGmst()
    {
        static const GMST staticGmst = [] {
            GMST gmst;

            const MWWorld::Store<ESM::GameSetting>& store
                = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

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

            return gmst;
        }();
        return staticGmst;
    }

    void Npc::ensureCustomData(const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            bool recalculate = false;
            auto tempData = std::make_unique<NpcCustomData>();
            NpcCustomData* data = tempData.get();
            MWMechanics::CreatureCustomDataResetter resetter{ ptr };
            ptr.getRefData().setCustomData(std::move(tempData));

            MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

            bool spellsInitialised = data->mNpcStats.getSpells().setSpells(ref->mBase->mId);

            // creature stats
            int gold = 0;
            if (ref->mBase->mNpdtType != ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
            {
                gold = ref->mBase->mNpdt.mGold;

                for (unsigned int i = 0; i < ESM::Skill::Length; ++i)
                    data->mNpcStats.getSkill(i).setBase(ref->mBase->mNpdt.mSkills[i]);

                data->mNpcStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mNpdt.mStrength);
                data->mNpcStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mNpdt.mIntelligence);
                data->mNpcStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mNpdt.mWillpower);
                data->mNpcStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mNpdt.mAgility);
                data->mNpcStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mNpdt.mSpeed);
                data->mNpcStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mNpdt.mEndurance);
                data->mNpcStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mNpdt.mPersonality);
                data->mNpcStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mNpdt.mLuck);

                data->mNpcStats.setHealth(ref->mBase->mNpdt.mHealth);
                data->mNpcStats.setMagicka(ref->mBase->mNpdt.mMana);
                data->mNpcStats.setFatigue(ref->mBase->mNpdt.mFatigue);

                data->mNpcStats.setLevel(ref->mBase->mNpdt.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt.mReputation);
            }
            else
            {
                gold = ref->mBase->mNpdt.mGold;

                for (int i = 0; i < 3; ++i)
                    data->mNpcStats.setDynamic(i, 10);

                data->mNpcStats.setLevel(ref->mBase->mNpdt.mLevel);
                data->mNpcStats.setBaseDisposition(ref->mBase->mNpdt.mDisposition);
                data->mNpcStats.setReputation(ref->mBase->mNpdt.mReputation);

                autoCalculateAttributes(ref->mBase, data->mNpcStats);
                autoCalculateSkills(ref->mBase, data->mNpcStats, ptr, spellsInitialised);

                recalculate = true;
            }

            // Persistent actors with 0 health do not play death animation
            if (data->mNpcStats.isDead())
                data->mNpcStats.setDeathAnimationFinished(isPersistent(ptr));

            // race powers
            const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(ref->mBase->mRace);
            data->mNpcStats.getSpells().addAllToInstance(race->mPowers.mList);

            if (!ref->mBase->mFaction.empty())
            {
                static const int iAutoRepFacMod = MWBase::Environment::get()
                                                      .getESMStore()
                                                      ->get<ESM::GameSetting>()
                                                      .find("iAutoRepFacMod")
                                                      ->mValue.getInteger();
                static const int iAutoRepLevMod = MWBase::Environment::get()
                                                      .getESMStore()
                                                      ->get<ESM::GameSetting>()
                                                      .find("iAutoRepLevMod")
                                                      ->mValue.getInteger();
                int rank = ref->mBase->getFactionRank();

                data->mNpcStats.setReputation(
                    iAutoRepFacMod * (rank + 1) + iAutoRepLevMod * (data->mNpcStats.getLevel() - 1));
            }

            data->mNpcStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mNpcStats.setAiSetting(MWMechanics::AiSetting::Hello, ref->mBase->mAiData.mHello);
            data->mNpcStats.setAiSetting(MWMechanics::AiSetting::Fight, ref->mBase->mAiData.mFight);
            data->mNpcStats.setAiSetting(MWMechanics::AiSetting::Flee, ref->mBase->mAiData.mFlee);
            data->mNpcStats.setAiSetting(MWMechanics::AiSetting::Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            if (!spellsInitialised)
                data->mNpcStats.getSpells().addAllToInstance(ref->mBase->mSpells.mList);

            data->mNpcStats.setGoldPool(gold);

            // store
            resetter.mPtr = {};
            if (recalculate)
                data->mNpcStats.recalculateMagicka();

            // inventory
            // setting ownership is used to make the NPC auto-equip his initial equipment only, and not bartered items
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            getInventoryStore(ptr).fill(ref->mBase->mInventory, ptr.getCellRef().getRefId(), prng);

            getInventoryStore(ptr).autoEquip();
        }
    }

    void Npc::insertObjectRendering(
        const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getObjects().insertNPC(ptr);
    }

    bool Npc::isPersistent(const MWWorld::ConstPtr& actor) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = actor.get<ESM::NPC>();
        return (ref->mBase->mRecordFlags & ESM::FLAG_Persistent) != 0;
    }

    std::string Npc::getModel(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        std::string model = Settings::Manager::getString("baseanim", "Models");
        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(ref->mBase->mRace);
        if (race->mData.mFlags & ESM::Race::Beast)
            model = Settings::Manager::getString("baseanimkna", "Models");

        return model;
    }

    void Npc::getModelsToPreload(const MWWorld::Ptr& ptr, std::vector<std::string>& models) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().search(npc->mBase->mRace);
        if (race && race->mData.mFlags & ESM::Race::Beast)
            models.emplace_back(Settings::Manager::getString("baseanimkna", "Models"));

        // keep these always loaded just in case
        models.emplace_back(Settings::Manager::getString("xargonianswimkna", "Models"));
        models.emplace_back(Settings::Manager::getString("xbaseanimfemale", "Models"));
        models.emplace_back(Settings::Manager::getString("xbaseanim", "Models"));

        const VFS::Manager* const vfs = MWBase::Environment::get().getResourceSystem()->getVFS();

        if (!npc->mBase->mModel.empty())
            models.push_back(Misc::ResourceHelpers::correctMeshPath(npc->mBase->mModel, vfs));

        if (!npc->mBase->mHead.empty())
        {
            const ESM::BodyPart* head
                = MWBase::Environment::get().getESMStore()->get<ESM::BodyPart>().search(npc->mBase->mHead);
            if (head)
                models.push_back(Misc::ResourceHelpers::correctMeshPath(head->mModel, vfs));
        }
        if (!npc->mBase->mHair.empty())
        {
            const ESM::BodyPart* hair
                = MWBase::Environment::get().getESMStore()->get<ESM::BodyPart>().search(npc->mBase->mHair);
            if (hair)
                models.push_back(Misc::ResourceHelpers::correctMeshPath(hair->mModel, vfs));
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
                if (equipped->getType() == ESM::Clothing::sRecordId)
                {
                    const ESM::Clothing* clothes = equipped->get<ESM::Clothing>()->mBase;
                    parts = clothes->mParts.mParts;
                }
                else if (equipped->getType() == ESM::Armor::sRecordId)
                {
                    const ESM::Armor* armor = equipped->get<ESM::Armor>()->mBase;
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
                    const ESM::RefId& partname
                        = (female && !it->mFemale.empty()) || (!female && it->mMale.empty()) ? it->mFemale : it->mMale;

                    const ESM::BodyPart* part
                        = MWBase::Environment::get().getESMStore()->get<ESM::BodyPart>().search(partname);
                    if (part && !part->mModel.empty())
                        models.push_back(Misc::ResourceHelpers::correctMeshPath(part->mModel, vfs));
                }
            }
        }

        // preload body parts
        if (race)
        {
            const std::vector<const ESM::BodyPart*>& parts
                = MWRender::NpcAnimation::getBodyParts(race->mId, female, false, false);
            for (std::vector<const ESM::BodyPart*>::const_iterator it = parts.begin(); it != parts.end(); ++it)
            {
                const ESM::BodyPart* part = *it;
                if (part && !part->mModel.empty())
                    models.push_back(Misc::ResourceHelpers::correctMeshPath(part->mModel, vfs));
            }
        }
    }

    std::string_view Npc::getName(const MWWorld::ConstPtr& ptr) const
    {
        if (ptr.getRefData().getCustomData()
            && ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats.isWerewolf())
        {
            const MWWorld::Store<ESM::GameSetting>& store
                = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

            return store.find("sWerewolfPopup")->mValue.getString();
        }

        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();
        const std::string& name = ref->mBase->mName;

        return !name.empty() ? name : ref->mBase->mId.getRefIdString();
    }

    MWMechanics::CreatureStats& Npc::getCreatureStats(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats;
    }

    MWMechanics::NpcStats& Npc::getNpcStats(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats;
    }

    bool Npc::evaluateHit(const MWWorld::Ptr& ptr, MWWorld::Ptr& victim, osg::Vec3f& hitPosition) const
    {
        victim = MWWorld::Ptr();
        hitPosition = osg::Vec3f();

        // Get the weapon used (if hand-to-hand, weapon = inv.end())
        MWWorld::InventoryStore& inv = getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        MWWorld::Ptr weapon;
        if (weaponslot != inv.end() && weaponslot->getType() == ESM::Weapon::sRecordId)
            weapon = *weaponslot;

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& store = world->getStore().get<ESM::GameSetting>();
        const float fCombatDistance = store.find("fCombatDistance")->mValue.getFloat();
        float dist = fCombatDistance
            * (!weapon.isEmpty() ? weapon.get<ESM::Weapon>()->mBase->mData.mReach
                                 : store.find("fHandToHandReach")->mValue.getFloat());

        // For AI actors, get combat targets to use in the ray cast. Only those targets will return a positive hit
        // result.
        std::vector<MWWorld::Ptr> targetActors;
        if (ptr != MWMechanics::getPlayer())
            getCreatureStats(ptr).getAiSequence().getCombatTargets(targetActors);

        // TODO: Use second to work out the hit angle
        std::pair<MWWorld::Ptr, osg::Vec3f> result = world->getHitContact(ptr, dist, targetActors);
        if (result.first.isEmpty()) // Didn't hit anything
            return true;

        const MWWorld::Class& othercls = result.first.getClass();
        if (!othercls.isActor()) // Can't hit non-actors
            return true;

        MWMechanics::CreatureStats& otherstats = othercls.getCreatureStats(result.first);
        if (otherstats.isDead()) // Can't hit dead actors
            return true;

        // Note that earlier we returned true in spite of an apparent failure to hit anything alive.
        // This is because hitting nothing is not a "miss" and should be handled as such character controller-side.
        victim = result.first;
        hitPosition = result.second;

        int weapskill = ESM::Skill::HandToHand;
        if (!weapon.isEmpty())
            weapskill = weapon.getClass().getEquipmentSkill(weapon);

        float hitchance = MWMechanics::getHitChance(ptr, victim, getSkill(ptr, weapskill));

        return Misc::Rng::roll0to99(world->getPrng()) < hitchance;
    }

    void Npc::hit(const MWWorld::Ptr& ptr, float attackStrength, int type, const MWWorld::Ptr& victim,
        const osg::Vec3f& hitPosition, bool success) const
    {
        MWWorld::InventoryStore& inv = getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator weaponslot = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        MWWorld::Ptr weapon;
        if (weaponslot != inv.end() && weaponslot->getType() == ESM::Weapon::sRecordId)
            weapon = *weaponslot;

        MWMechanics::applyFatigueLoss(ptr, weapon, attackStrength);

        if (victim.isEmpty()) // Didn't hit anything
            return;

        const MWWorld::Class& othercls = victim.getClass();
        MWMechanics::CreatureStats& otherstats = othercls.getCreatureStats(victim);
        if (otherstats.isDead()) // Can't hit dead actors
            return;

        if (ptr == MWMechanics::getPlayer())
            MWBase::Environment::get().getWindowManager()->setEnemy(victim);

        if (!success)
        {
            othercls.onHit(victim, 0.0f, false, weapon, ptr, osg::Vec3f(), false);
            MWMechanics::reduceWeaponCondition(0.f, false, weapon, ptr);
            return;
        }

        bool healthdmg;
        float damage = 0.0f;
        if (!weapon.isEmpty())
        {
            const unsigned char* attack = nullptr;
            if (type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
            else if (type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash;
            else if (type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust;
            if (attack)
            {
                damage = attack[0] + ((attack[1] - attack[0]) * attackStrength);
            }
            MWMechanics::adjustWeaponDamage(damage, weapon, ptr);
            MWMechanics::reduceWeaponCondition(damage, true, weapon, ptr);
            MWMechanics::resistNormalWeapon(victim, ptr, weapon, damage);
            MWMechanics::applyWerewolfDamageMult(victim, weapon, damage);
            healthdmg = true;
        }
        else
        {
            MWMechanics::getHandToHandDamage(ptr, victim, damage, healthdmg, attackStrength);
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& store = world->getStore().get<ESM::GameSetting>();

        if (ptr == MWMechanics::getPlayer())
        {
            int weapskill = ESM::Skill::HandToHand;
            if (!weapon.isEmpty())
                weapskill = weapon.getClass().getEquipmentSkill(weapon);
            skillUsageSucceeded(ptr, weapskill, 0);

            const MWMechanics::AiSequence& seq = victim.getClass().getCreatureStats(victim).getAiSequence();

            bool unaware
                = !seq.isInCombat() && !MWBase::Environment::get().getMechanicsManager()->awarenessCheck(ptr, victim);
            if (unaware)
            {
                damage *= store.find("fCombatCriticalStrikeMult")->mValue.getFloat();
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTargetCriticalStrike}");
                MWBase::Environment::get().getSoundManager()->playSound3D(
                    victim, ESM::RefId::stringRefId("critical damage"), 1.0f, 1.0f);
            }
        }

        if (othercls.getCreatureStats(victim).getKnockedDown())
            damage *= store.find("fCombatKODamageMult")->mValue.getFloat();

        // Apply "On hit" enchanted weapons
        MWMechanics::applyOnStrikeEnchantment(ptr, victim, weapon, hitPosition);

        MWMechanics::applyElementalShields(ptr, victim);

        if (MWMechanics::blockMeleeAttack(ptr, victim, weapon, damage, attackStrength))
        {
            damage = 0;
            victim.getClass().block(victim);
        }

        if (victim == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState())
            damage = 0;

        MWMechanics::diseaseContact(victim, ptr);

        othercls.onHit(victim, damage, healthdmg, weapon, ptr, hitPosition, true);
    }

    void Npc::onHit(const MWWorld::Ptr& ptr, float damage, bool ishealth, const MWWorld::Ptr& object,
        const MWWorld::Ptr& attacker, const osg::Vec3f& hitPosition, bool successful) const
    {
        MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
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
                && (statsAttacker.getAiSequence().isInCombat(ptr) || attacker == MWMechanics::getPlayer()))
                stats.setHitAttemptActorId(statsAttacker.getActorId());

            // Next handle the attacking actor
            if ((statsAttacker.getHitAttemptActorId() == -1)
                && (statsAttacker.getAiSequence().isInCombat(ptr) || attacker == MWMechanics::getPlayer()))
                statsAttacker.setHitAttemptActorId(stats.getActorId());
        }

        if (!object.isEmpty())
            stats.setLastHitAttemptObject(object.getCellRef().getRefId());

        if (setOnPcHitMe && !attacker.isEmpty() && attacker == MWMechanics::getPlayer())
        {
            const ESM::RefId& script = getScript(ptr);
            /* Set the OnPCHitMe script variable. The script is responsible for clearing it. */
            if (!script.empty())
                ptr.getRefData().getLocals().setVarByInt(script, "onpchitme", 1);
        }

        if (!successful)
        {
            // Missed
            if (!attacker.isEmpty() && attacker == MWMechanics::getPlayer())
                sndMgr->playSound3D(ptr, ESM::RefId::stringRefId("miss"), 1.0f, 1.0f);
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

            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            const GMST& gmst = getGmst();

            int chance = store.get<ESM::GameSetting>().find("iVoiceHitOdds")->mValue.getInteger();
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            if (Misc::Rng::roll0to99(prng) < chance)
                MWBase::Environment::get().getDialogueManager()->say(ptr, ESM::RefId::stringRefId("hit"));

            // Check for knockdown
            float agilityTerm
                = stats.getAttribute(ESM::Attribute::Agility).getModified() * gmst.fKnockDownMult->mValue.getFloat();
            float knockdownTerm = stats.getAttribute(ESM::Attribute::Agility).getModified()
                    * gmst.iKnockDownOddsMult->mValue.getInteger() * 0.01f
                + gmst.iKnockDownOddsBase->mValue.getInteger();
            if (ishealth && agilityTerm <= damage && knockdownTerm <= Misc::Rng::roll0to99(prng))
                stats.setKnockedDown(true);
            else
                stats.setHitRecovery(true); // Is this supposed to always occur?

            if (damage > 0 && ishealth)
            {
                // Hit percentages:
                // cuirass = 30%
                // shield, helmet, greaves, boots, pauldrons = 10% each
                // guantlets = 5% each
                static const int hitslots[20]
                    = { MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                          MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                          MWWorld::InventoryStore::Slot_Cuirass, MWWorld::InventoryStore::Slot_Cuirass,
                          MWWorld::InventoryStore::Slot_CarriedLeft, MWWorld::InventoryStore::Slot_CarriedLeft,
                          MWWorld::InventoryStore::Slot_Helmet, MWWorld::InventoryStore::Slot_Helmet,
                          MWWorld::InventoryStore::Slot_Greaves, MWWorld::InventoryStore::Slot_Greaves,
                          MWWorld::InventoryStore::Slot_Boots, MWWorld::InventoryStore::Slot_Boots,
                          MWWorld::InventoryStore::Slot_LeftPauldron, MWWorld::InventoryStore::Slot_LeftPauldron,
                          MWWorld::InventoryStore::Slot_RightPauldron, MWWorld::InventoryStore::Slot_RightPauldron,
                          MWWorld::InventoryStore::Slot_LeftGauntlet, MWWorld::InventoryStore::Slot_RightGauntlet };
                int hitslot = hitslots[Misc::Rng::rollDice(20, prng)];

                float unmitigatedDamage = damage;
                float x = damage / (damage + getArmorRating(ptr));
                damage *= std::max(gmst.fCombatArmorMinMult->mValue.getFloat(), x);
                int damageDiff = static_cast<int>(unmitigatedDamage - damage);
                damage = std::max(1.f, damage);
                damageDiff = std::max(1, damageDiff);

                MWWorld::InventoryStore& inv = getInventoryStore(ptr);
                MWWorld::ContainerStoreIterator armorslot = inv.getSlot(hitslot);
                MWWorld::Ptr armor = ((armorslot != inv.end()) ? *armorslot : MWWorld::Ptr());
                bool hasArmor = !armor.isEmpty() && armor.getType() == ESM::Armor::sRecordId;
                // If there's no item in the carried left slot or if it is not a shield redistribute the hit.
                if (!hasArmor && hitslot == MWWorld::InventoryStore::Slot_CarriedLeft)
                {
                    if (Misc::Rng::rollDice(2, prng) == 0)
                        hitslot = MWWorld::InventoryStore::Slot_Cuirass;
                    else
                        hitslot = MWWorld::InventoryStore::Slot_LeftPauldron;
                    armorslot = inv.getSlot(hitslot);
                    if (armorslot != inv.end())
                    {
                        armor = *armorslot;
                        hasArmor = !armor.isEmpty() && armor.getType() == ESM::Armor::sRecordId;
                    }
                }
                if (hasArmor)
                {
                    static const bool creatureDamage
                        = Settings::Manager::getBool("unarmed creature attacks damage armor", "Game");

                    if (!object.isEmpty() || attacker.isEmpty() || attacker.getClass().isNpc()
                        || creatureDamage) // Unarmed creature attacks don't affect armor condition unless it was
                                           // explicitly requested.
                    {
                        int armorhealth = armor.getClass().getItemHealth(armor);
                        armorhealth -= std::min(damageDiff, armorhealth);
                        armor.getCellRef().setCharge(armorhealth);

                        // Armor broken? unequip it
                        if (armorhealth == 0)
                            armor = *inv.unequipItem(armor);
                    }

                    if (ptr == MWMechanics::getPlayer())
                        skillUsageSucceeded(ptr, armor.getClass().getEquipmentSkill(armor), 0);

                    switch (armor.getClass().getEquipmentSkill(armor))
                    {
                        case ESM::Skill::LightArmor:
                            sndMgr->playSound3D(ptr, ESM::RefId::stringRefId("Light Armor Hit"), 1.0f, 1.0f);
                            break;
                        case ESM::Skill::MediumArmor:
                            sndMgr->playSound3D(ptr, ESM::RefId::stringRefId("Medium Armor Hit"), 1.0f, 1.0f);
                            break;
                        case ESM::Skill::HeavyArmor:
                            sndMgr->playSound3D(ptr, ESM::RefId::stringRefId("Heavy Armor Hit"), 1.0f, 1.0f);
                            break;
                    }
                }
                else if (ptr == MWMechanics::getPlayer())
                    skillUsageSucceeded(ptr, ESM::Skill::Unarmored, 0);
            }
        }

        if (ishealth)
        {
            if (!attacker.isEmpty() && !godmode)
                damage = scaleDamage(damage, attacker, ptr);

            if (damage > 0.0f)
            {
                sndMgr->playSound3D(ptr, ESM::RefId::stringRefId("Health Damage"), 1.0f, 1.0f);
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
            if (!attacker.isEmpty() && attacker.getClass().isNpc()
                && attacker.getClass().getNpcStats(attacker).isWerewolf())
            {
                attacker.getClass().getNpcStats(attacker).addWerewolfKill();
            }

            MWBase::Environment::get().getMechanicsManager()->actorKilled(ptr, attacker);
        }
    }

    std::unique_ptr<MWWorld::Action> Npc::activate(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor) const
    {
        // player got activated by another NPC
        if (ptr == MWMechanics::getPlayer())
            return std::make_unique<MWWorld::ActionTalk>(actor);

        // Werewolfs can't activate NPCs
        if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Sound* sound = store.get<ESM::Sound>().searchRandom("WolfNPC", prng);

            std::unique_ptr<MWWorld::Action> action = std::make_unique<MWWorld::FailedAction>("#{sWerewolfRefusal}");
            if (sound)
                action->setSound(sound->mId);

            return action;
        }

        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);

        if (stats.isDead())
        {
            bool canLoot = Settings::Manager::getBool("can loot during death animation", "Game");

            // by default user can loot friendly actors during death animation
            if (canLoot && !stats.getAiSequence().isInCombat())
                return std::make_unique<MWWorld::ActionOpen>(ptr);

            // otherwise wait until death animation
            if (stats.isDeathAnimationFinished())
                return std::make_unique<MWWorld::ActionOpen>(ptr);
        }
        else if (!stats.getAiSequence().isInCombat())
        {
            if (stats.getKnockedDown() || MWBase::Environment::get().getMechanicsManager()->isSneaking(actor))
                return std::make_unique<MWWorld::ActionOpen>(ptr); // stealing

            // Can't talk to werewolves
            if (!getNpcStats(ptr).isWerewolf())
                return std::make_unique<MWWorld::ActionTalk>(ptr);
        }
        else // In combat
        {
            const bool stealingInCombat
                = Settings::Manager::getBool("always allow stealing from knocked out actors", "Game");
            if (stealingInCombat && stats.getKnockedDown())
                return std::make_unique<MWWorld::ActionOpen>(ptr); // stealing
        }

        // Tribunal and some mod companions oddly enough must use open action as fallback
        if (!getScript(ptr).empty() && ptr.getRefData().getLocals().getIntVar(getScript(ptr), "companion"))
            return std::make_unique<MWWorld::ActionOpen>(ptr);

        return std::make_unique<MWWorld::FailedAction>();
    }

    MWWorld::ContainerStore& Npc::getContainerStore(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);
        auto& store = ptr.getRefData().getCustomData()->asNpcCustomData().mInventoryStore;
        store.setActor(ptr);
        return store;
    }

    MWWorld::InventoryStore& Npc::getInventoryStore(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);
        auto& store = ptr.getRefData().getCustomData()->asNpcCustomData().mInventoryStore;
        store.setActor(ptr);
        return store;
    }

    ESM::RefId Npc::getScript(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        return ref->mBase->mScript;
    }

    float Npc::getMaxSpeed(const MWWorld::Ptr& ptr) const
    {
        // TODO: This function is called several times per frame for each NPC.
        // It would be better to calculate it only once per frame for each NPC and save the result in CreatureStats.
        const MWMechanics::NpcStats& stats = getNpcStats(ptr);
        bool godmode = ptr == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();
        if ((!godmode && stats.isParalyzed()) || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const MWBase::World* world = MWBase::Environment::get().getWorld();
        const GMST& gmst = getGmst();

        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();

        const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);

        bool swimming = world->isSwimming(ptr);
        bool sneaking = MWBase::Environment::get().getMechanicsManager()->isSneaking(ptr);
        bool running = stats.getStance(MWMechanics::CreatureStats::Stance_Run);
        bool inair = !world->isOnGround(ptr) && !swimming && !world->isFlying(ptr);
        running = running && (inair || MWBase::Environment::get().getMechanicsManager()->isRunning(ptr));

        float moveSpeed;
        if (getEncumbrance(ptr) > getCapacity(ptr))
            moveSpeed = 0.0f;
        else if (mageffects.get(ESM::MagicEffect::Levitate).getMagnitude() > 0 && world->isLevitationEnabled())
        {
            float flySpeed = 0.01f
                * (stats.getAttribute(ESM::Attribute::Speed).getModified()
                    + mageffects.get(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->mValue.getFloat()
                + flySpeed * (gmst.fMaxFlySpeed->mValue.getFloat() - gmst.fMinFlySpeed->mValue.getFloat());
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if (swimming)
            moveSpeed = getSwimSpeed(ptr);
        else if (running && !sneaking)
            moveSpeed = getRunSpeed(ptr);
        else
            moveSpeed = getWalkSpeed(ptr);

        if (stats.isWerewolf() && running && stats.getDrawState() == MWMechanics::DrawState::Nothing)
            moveSpeed *= gmst.fWereWolfRunMult->mValue.getFloat();

        return moveSpeed;
    }

    float Npc::getJump(const MWWorld::Ptr& ptr) const
    {
        if (getEncumbrance(ptr) > getCapacity(ptr))
            return 0.f;

        const MWMechanics::NpcStats& stats = getNpcStats(ptr);
        bool godmode = ptr == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();
        if ((!godmode && stats.isParalyzed()) || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const GMST& gmst = getGmst();
        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();
        const float encumbranceTerm = gmst.fJumpEncumbranceBase->mValue.getFloat()
            + gmst.fJumpEncumbranceMultiplier->mValue.getFloat() * (1.0f - Npc::getNormalizedEncumbrance(ptr));

        float a = getSkill(ptr, ESM::Skill::Acrobatics);
        float b = 0.0f;
        if (a > 50.0f)
        {
            b = a - 50.0f;
            a = 50.0f;
        }

        float x = gmst.fJumpAcrobaticsBase->mValue.getFloat()
            + std::pow(a / 15.0f, gmst.fJumpAcroMultiplier->mValue.getFloat());
        x += 3.0f * b * gmst.fJumpAcroMultiplier->mValue.getFloat();
        x += mageffects.get(ESM::MagicEffect::Jump).getMagnitude() * 64;
        x *= encumbranceTerm;

        if (stats.getStance(MWMechanics::CreatureStats::Stance_Run))
            x *= gmst.fJumpRunMultiplier->mValue.getFloat();
        x *= stats.getFatigueTerm();
        x -= -Constants::GravityConst * Constants::UnitsPerMeter;
        x /= 3.0f;

        return x;
    }

    MWMechanics::Movement& Npc::getMovementSettings(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);

        return ptr.getRefData().getCustomData()->asNpcCustomData().mMovement;
    }

    bool Npc::isEssential(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        return (ref->mBase->mFlags & ESM::NPC::Essential) != 0;
    }

    bool Npc::hasToolTip(const MWWorld::ConstPtr& ptr) const
    {
        if (!ptr.getRefData().getCustomData() || MWBase::Environment::get().getWindowManager()->isGuiMode())
            return true;

        const NpcCustomData& customData = ptr.getRefData().getCustomData()->asNpcCustomData();

        if (customData.mNpcStats.isDead() && customData.mNpcStats.isDeathAnimationFinished())
            return true;

        if (!customData.mNpcStats.getAiSequence().isInCombat())
            return true;

        const bool stealingInCombat
            = Settings::Manager::getBool("always allow stealing from knocked out actors", "Game");
        if (stealingInCombat && customData.mNpcStats.getKnockedDown())
            return true;

        return false;
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MWGui::toUString(name));
        if (fullHelp && !ref->mBase->mName.empty() && ptr.getRefData().getCustomData()
            && ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats.isWerewolf())
        {
            info.caption += " (";
            info.caption += MyGUI::TextIterator::toTagsString(ref->mBase->mName);
            info.caption += ")";
        }

        if (fullHelp)
            info.text = MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");

        return info;
    }

    float Npc::getCapacity(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        static const float fEncumbranceStrMult = MWBase::Environment::get()
                                                     .getESMStore()
                                                     ->get<ESM::GameSetting>()
                                                     .find("fEncumbranceStrMult")
                                                     ->mValue.getFloat();
        return stats.getAttribute(ESM::Attribute::Strength).getModified() * fEncumbranceStrMult;
    }

    float Npc::getEncumbrance(const MWWorld::Ptr& ptr) const
    {
        // According to UESP, inventory weight is ignored in werewolf form. Does that include
        // feather and burden effects?
        return getNpcStats(ptr).isWerewolf() ? 0.0f : Actor::getEncumbrance(ptr);
    }

    bool Npc::consume(const MWWorld::Ptr& consumable, const MWWorld::Ptr& actor) const
    {
        MWBase::Environment::get().getWorld()->breakInvisibility(actor);
        MWMechanics::CastSpell cast(actor, actor);
        const ESM::RefId& recordId = consumable.getCellRef().getRefId();
        MWBase::Environment::get().getLuaManager()->itemConsumed(consumable, actor);
        actor.getClass().getContainerStore(actor).remove(consumable, 1);
        return cast.cast(recordId);
    }

    void Npc::skillUsageSucceeded(const MWWorld::Ptr& ptr, int skill, int usageType, float extraFactor) const
    {
        MWMechanics::NpcStats& stats = getNpcStats(ptr);

        if (stats.isWerewolf())
            return;

        MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        const ESM::Class* class_ = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(ref->mBase->mClass);

        stats.useSkill(skill, *class_, usageType, extraFactor);
    }

    float Npc::getArmorRating(const MWWorld::Ptr& ptr) const
    {
        const MWWorld::Store<ESM::GameSetting>& store
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        MWMechanics::NpcStats& stats = getNpcStats(ptr);
        const MWWorld::InventoryStore& invStore = getInventoryStore(ptr);

        float fUnarmoredBase1 = store.find("fUnarmoredBase1")->mValue.getFloat();
        float fUnarmoredBase2 = store.find("fUnarmoredBase2")->mValue.getFloat();
        float unarmoredSkill = getSkill(ptr, ESM::Skill::Unarmored);

        float ratings[MWWorld::InventoryStore::Slots];
        for (int i = 0; i < MWWorld::InventoryStore::Slots; i++)
        {
            MWWorld::ConstContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getType() != ESM::Armor::sRecordId)
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
                  + ratings[MWWorld::InventoryStore::Slot_LeftPauldron]
                  + ratings[MWWorld::InventoryStore::Slot_RightPauldron])
            * 0.1f
            + (ratings[MWWorld::InventoryStore::Slot_LeftGauntlet]
                  + ratings[MWWorld::InventoryStore::Slot_RightGauntlet])
            * 0.05f
            + shield;
    }

    void Npc::adjustScale(const MWWorld::ConstPtr& ptr, osg::Vec3f& scale, bool rendering) const
    {
        if (!rendering)
            return; // collision meshes are not scaled based on race height
                    // having the same collision extents for all races makes the environments easier to test

        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(ref->mBase->mRace);

        // Race weight should not affect 1st-person meshes, otherwise it will change hand proportions and can break
        // aiming.
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

    int Npc::getServices(const MWWorld::ConstPtr& actor) const
    {
        const ESM::NPC* npc = actor.get<ESM::NPC>()->mBase;
        if (npc->mFlags & ESM::NPC::Autocalc)
        {
            const ESM::Class* class_ = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);
            return class_->mData.mServices;
        }
        return npc->mAiData.mServices;
    }

    ESM::RefId Npc::getSoundIdFromSndGen(const MWWorld::Ptr& ptr, std::string_view name) const
    {
        static const ESM::RefId swimLeft = ESM::RefId::stringRefId("Swim Left");
        static const ESM::RefId swimRight = ESM::RefId::stringRefId("Swim Right");
        static const ESM::RefId footWaterLeft = ESM::RefId::stringRefId("FootWaterLeft");
        static const ESM::RefId footWaterRight = ESM::RefId::stringRefId("FootWaterRight");
        static const ESM::RefId footBareLeft = ESM::RefId::stringRefId("FootBareLeft");
        static const ESM::RefId footBareRight = ESM::RefId::stringRefId("FootBareRight");
        static const ESM::RefId footLightLeft = ESM::RefId::stringRefId("footLightLeft");
        static const ESM::RefId footLightRight = ESM::RefId::stringRefId("footLightRight");
        static const ESM::RefId footMediumRight = ESM::RefId::stringRefId("FootMedRight");
        static const ESM::RefId footMediumLeft = ESM::RefId::stringRefId("FootMedLeft");
        static const ESM::RefId footHeavyLeft = ESM::RefId::stringRefId("footHeavyLeft");
        static const ESM::RefId footHeavyRight = ESM::RefId::stringRefId("footHeavyRight");

        if (name == "left" || name == "right")
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            if (world->isFlying(ptr))
                return ESM::RefId();
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if (world->isSwimming(ptr))
                return (name == "left") ? swimLeft : swimRight;
            if (world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return (name == "left") ? footWaterLeft : footWaterRight;
            if (world->isOnGround(ptr))
            {
                if (getNpcStats(ptr).isWerewolf()
                    && getCreatureStats(ptr).getStance(MWMechanics::CreatureStats::Stance_Run))
                {
                    int weaponType = ESM::Weapon::None;
                    MWMechanics::getActiveWeapon(ptr, &weaponType);
                    if (weaponType == ESM::Weapon::None)
                        return ESM::RefId();
                }

                const MWWorld::InventoryStore& inv = Npc::getInventoryStore(ptr);
                MWWorld::ConstContainerStoreIterator boots = inv.getSlot(MWWorld::InventoryStore::Slot_Boots);
                if (boots == inv.end() || boots->getType() != ESM::Armor::sRecordId)
                    return (name == "left") ? footBareLeft : footBareRight;

                switch (boots->getClass().getEquipmentSkill(*boots))
                {
                    case ESM::Skill::LightArmor:
                        return (name == "left") ? footLightLeft : footLightRight;
                        break;
                    case ESM::Skill::MediumArmor:
                        return (name == "left") ? footMediumLeft : footMediumRight;
                        break;
                    case ESM::Skill::HeavyArmor:
                        return (name == "left") ? footHeavyLeft : footHeavyRight;
                        break;
                }
            }
            return ESM::RefId();
        }

        // Morrowind ignores land soundgen for NPCs
        if (name == "land")
            return ESM::RefId();
        if (name == "swimleft")
            return swimLeft;
        if (name == "swimright")
            return swimRight;
        // TODO: I have no idea what these are supposed to do for NPCs since they use
        // voiced dialog for various conditions like health loss and combat taunts. Maybe
        // only for biped creatures?

        if (name == "moan")
            return ESM::RefId();
        if (name == "roar")
            return ESM::RefId();
        if (name == "scream")
            return ESM::RefId();

        throw std::runtime_error("Unexpected soundgen type: " + std::string(name));
    }

    MWWorld::Ptr Npc::copyToCellImpl(const MWWorld::ConstPtr& ptr, MWWorld::CellStore& cell) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        return MWWorld::Ptr(cell.insert(ref), &cell);
    }

    float Npc::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        return getNpcStats(ptr).getSkill(skill).getModified();
    }

    int Npc::getBloodTexture(const MWWorld::ConstPtr& ptr) const
    {
        return ptr.get<ESM::NPC>()->mBase->mBloodType;
    }

    void Npc::readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::NpcState& npcState = state.asNpcState();

        if (state.mVersion > 0)
        {
            if (!ptr.getRefData().getCustomData())
            {
                if (npcState.mCreatureStats.mMissingACDT)
                    ensureCustomData(ptr);
                else
                    // Create a CustomData, but don't fill it from ESM records (not needed)
                    ptr.getRefData().setCustomData(std::make_unique<NpcCustomData>());
            }
        }
        else
            ensureCustomData(
                ptr); // in openmw 0.30 savegames not all state was saved yet, so need to load it regardless.

        NpcCustomData& customData = ptr.getRefData().getCustomData()->asNpcCustomData();

        customData.mInventoryStore.readState(npcState.mInventory);
        customData.mNpcStats.readState(npcState.mNpcStats);
        bool spellsInitialised = customData.mNpcStats.getSpells().setSpells(ptr.get<ESM::NPC>()->mBase->mId);
        if (spellsInitialised)
            customData.mNpcStats.getSpells().clear();
        customData.mNpcStats.readState(npcState.mCreatureStats);
    }

    void Npc::writeAdditionalState(const MWWorld::ConstPtr& ptr, ESM::ObjectState& state) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            state.mHasCustomState = false;
            return;
        }

        const NpcCustomData& customData = ptr.getRefData().getCustomData()->asNpcCustomData();
        if (ptr.getRefData().getCount() <= 0
            && (!(ptr.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Respawn) || !customData.mNpcStats.isDead()))
        {
            state.mHasCustomState = false;
            return;
        }

        ESM::NpcState& npcState = state.asNpcState();
        customData.mInventoryStore.writeState(npcState.mInventory);
        customData.mNpcStats.writeState(npcState.mNpcStats);
        customData.mNpcStats.writeState(npcState.mCreatureStats);
    }

    int Npc::getBaseGold(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();
        return ref->mBase->mNpdt.mGold;
    }

    bool Npc::isClass(const MWWorld::ConstPtr& ptr, std::string_view className) const
    {
        return ptr.get<ESM::NPC>()->mBase->mClass == className;
    }

    bool Npc::canSwim(const MWWorld::ConstPtr& ptr) const
    {
        return true;
    }

    bool Npc::canWalk(const MWWorld::ConstPtr& ptr) const
    {
        return true;
    }

    void Npc::respawn(const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::CreatureStats& creatureStats = getCreatureStats(ptr);
        if (ptr.getRefData().getCount() > 0 && !creatureStats.isDead())
            return;

        if (!creatureStats.isDeathAnimationFinished())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
        static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

        float delay
            = ptr.getRefData().getCount() == 0 ? fCorpseClearDelay : std::min(fCorpseRespawnDelay, fCorpseClearDelay);

        if (ptr.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Respawn
            && creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getRefData().getCount() == 0)
                {
                    ptr.getRefData().setCount(1);
                    const ESM::RefId& script = getScript(ptr);
                    if (!script.empty())
                        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, ptr);
                }

                MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                MWBase::Environment::get().getWindowManager()->onDeleteCustomData(ptr);
                ptr.getRefData().setCustomData(nullptr);

                // Reset to original position
                MWBase::Environment::get().getWorld()->moveObject(ptr, ptr.getCellRef().getPosition().asVec3());
                MWBase::Environment::get().getWorld()->rotateObject(
                    ptr, ptr.getCellRef().getPosition().asRotationVec3(), MWBase::RotationFlag_none);
            }
        }
    }

    int Npc::getBaseFightRating(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();
        return ref->mBase->mAiData.mFight;
    }

    bool Npc::isBipedal(const MWWorld::ConstPtr& ptr) const
    {
        return true;
    }

    ESM::RefId Npc::getPrimaryFaction(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();
        return ref->mBase->mFaction;
    }

    int Npc::getPrimaryFactionRank(const MWWorld::ConstPtr& ptr) const
    {
        const ESM::RefId& factionID = ptr.getClass().getPrimaryFaction(ptr);
        if (factionID.empty())
            return -1;

        // Search in the NPC data first
        if (const MWWorld::CustomData* data = ptr.getRefData().getCustomData())
        {
            int rank = data->asNpcCustomData().mNpcStats.getFactionRank(factionID);
            if (rank >= 0)
                return rank;
        }

        // Use base NPC record as a fallback
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();
        return ref->mBase->getFactionRank();
    }

    void Npc::setBaseAISetting(const ESM::RefId& id, MWMechanics::AiSetting setting, int value) const
    {
        MWMechanics::setBaseAISetting<ESM::NPC>(id, setting, value);
    }

    void Npc::modifyBaseInventory(const ESM::RefId& actorId, const ESM::RefId& itemId, int amount) const
    {
        MWMechanics::modifyBaseInventory<ESM::NPC>(actorId, itemId, amount);
    }

    float Npc::getWalkSpeed(const MWWorld::Ptr& ptr) const
    {
        const GMST& gmst = getGmst();
        const MWMechanics::NpcStats& stats = getNpcStats(ptr);
        const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);
        const bool sneaking = MWBase::Environment::get().getMechanicsManager()->isSneaking(ptr);

        float walkSpeed = gmst.fMinWalkSpeed->mValue.getFloat()
            + 0.01f * stats.getAttribute(ESM::Attribute::Speed).getModified()
                * (gmst.fMaxWalkSpeed->mValue.getFloat() - gmst.fMinWalkSpeed->mValue.getFloat());
        walkSpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat() * normalizedEncumbrance;
        walkSpeed = std::max(0.0f, walkSpeed);
        if (sneaking)
            walkSpeed *= gmst.fSneakSpeedMultiplier->mValue.getFloat();

        return walkSpeed;
    }

    float Npc::getRunSpeed(const MWWorld::Ptr& ptr) const
    {
        const GMST& gmst = getGmst();
        return getWalkSpeed(ptr)
            * (0.01f * getSkill(ptr, ESM::Skill::Athletics) * gmst.fAthleticsRunBonus->mValue.getFloat()
                + gmst.fBaseRunMultiplier->mValue.getFloat());
    }

    float Npc::getSwimSpeed(const MWWorld::Ptr& ptr) const
    {
        const MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWMechanics::NpcStats& stats = getNpcStats(ptr);
        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();
        const bool swimming = world->isSwimming(ptr);
        const bool inair = !world->isOnGround(ptr) && !swimming && !world->isFlying(ptr);
        const bool running = stats.getStance(MWMechanics::CreatureStats::Stance_Run)
            && (inair || MWBase::Environment::get().getMechanicsManager()->isRunning(ptr));

        return getSwimSpeedImpl(ptr, getGmst(), mageffects, running ? getRunSpeed(ptr) : getWalkSpeed(ptr));
    }
}
