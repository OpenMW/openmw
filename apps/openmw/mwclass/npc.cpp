#include "npc.hpp"

#include <MyGUI_TextIterator.h>
#include <MyGUI_UString.h>

#include <cassert>
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
#include <components/settings/values.hpp>
#include <components/vfs/pathutil.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwlua/localscripts.hpp"

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
#include "../mwworld/worldmodel.hpp"

#include "../mwrender/npcanimation.hpp"
#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

#include "nameorid.hpp"

namespace
{
    struct NpcParts
    {
        const ESM::RefId mSwimLeft = ESM::RefId::stringRefId("Swim Left");
        const ESM::RefId mSwimRight = ESM::RefId::stringRefId("Swim Right");
        const ESM::RefId mFootWaterLeft = ESM::RefId::stringRefId("FootWaterLeft");
        const ESM::RefId mFootWaterRight = ESM::RefId::stringRefId("FootWaterRight");
        const ESM::RefId mFootBareLeft = ESM::RefId::stringRefId("FootBareLeft");
        const ESM::RefId mFootBareRight = ESM::RefId::stringRefId("FootBareRight");
        const ESM::RefId mFootLightLeft = ESM::RefId::stringRefId("footLightLeft");
        const ESM::RefId mFootLightRight = ESM::RefId::stringRefId("footLightRight");
        const ESM::RefId mFootMediumRight = ESM::RefId::stringRefId("FootMedRight");
        const ESM::RefId mFootMediumLeft = ESM::RefId::stringRefId("FootMedLeft");
        const ESM::RefId mFootHeavyLeft = ESM::RefId::stringRefId("footHeavyLeft");
        const ESM::RefId mFootHeavyRight = ESM::RefId::stringRefId("footHeavyRight");
    };

    const NpcParts npcParts;

    int is_even(double d)
    {
        double intPart;
        modf(d / 2.0, &intPart);
        return 2.0 * intPart == d;
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

        const auto& attributes = MWBase::Environment::get().getESMStore()->get<ESM::Attribute>();
        int level = creatureStats.getLevel();
        for (const ESM::Attribute& attribute : attributes)
            creatureStats.setAttribute(attribute.mId, race->mData.getAttribute(attribute.mId, male));

        // class bonus
        const ESM::Class* npcClass = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);

        for (int attribute : npcClass->mData.mAttribute)
        {
            if (attribute >= 0 && attribute < ESM::Attribute::Length)
            {
                auto id = ESM::Attribute::indexToRefId(attribute);
                creatureStats.setAttribute(id, creatureStats.getAttribute(id).getBase() + 10);
            }
        }

        // skill bonus
        for (const ESM::Attribute& attribute : attributes)
        {
            float modifierSum = 0;
            int attributeIndex = ESM::Attribute::refIdToIndex(attribute.mId);

            for (const ESM::Skill& skill : MWBase::Environment::get().getESMStore()->get<ESM::Skill>())
            {
                if (skill.mData.mAttribute != attributeIndex)
                    continue;

                // is this a minor or major skill?
                float add = 0.2f;
                int index = ESM::Skill::refIdToIndex(skill.mId);
                for (const auto& skills : npcClass->mData.mSkills)
                {
                    if (skills[0] == index)
                        add = 0.5;
                    if (skills[1] == index)
                        add = 1.0;
                }
                modifierSum += add;
            }
            creatureStats.setAttribute(attribute.mId,
                std::min(
                    round_ieee_754(creatureStats.getAttribute(attribute.mId).getBase() + (level - 1) * modifierSum),
                    100));
        }

        // initial health
        float strength = creatureStats.getAttribute(ESM::Attribute::Strength).getBase();
        float endurance = creatureStats.getAttribute(ESM::Attribute::Endurance).getBase();

        int multiplier = 3;

        if (npcClass->mData.mSpecialization == ESM::Class::Combat)
            multiplier += 2;
        else if (npcClass->mData.mSpecialization == ESM::Class::Stealth)
            multiplier += 1;

        if (std::find(npcClass->mData.mAttribute.begin(), npcClass->mData.mAttribute.end(),
                ESM::Attribute::refIdToIndex(ESM::Attribute::Endurance))
            != npcClass->mData.mAttribute.end())
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
        const ESM::Class* npcClass = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);

        unsigned int level = npcStats.getLevel();

        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(npc->mRace);

        for (int i = 0; i < 2; ++i)
        {
            int bonus = (i == 0) ? 10 : 25;

            for (const auto& skills : npcClass->mData.mSkills)
            {
                ESM::RefId id = ESM::Skill::indexToRefId(skills[i]);
                if (!id.empty())
                {
                    npcStats.getSkill(id).setBase(npcStats.getSkill(id).getBase() + bonus);
                }
            }
        }

        for (const ESM::Skill& skill : MWBase::Environment::get().getESMStore()->get<ESM::Skill>())
        {
            float majorMultiplier = 0.1f;
            float specMultiplier = 0.0f;

            int raceBonus = 0;
            int specBonus = 0;

            int index = ESM::Skill::refIdToIndex(skill.mId);
            auto bonusIt = std::find_if(race->mData.mBonus.begin(), race->mData.mBonus.end(),
                [&](const auto& bonus) { return bonus.mSkill == index; });
            if (bonusIt != race->mData.mBonus.end())
                raceBonus = bonusIt->mBonus;

            for (const auto& skills : npcClass->mData.mSkills)
            {
                // is this a minor or major skill?
                if (std::find(skills.begin(), skills.end(), index) != skills.end())
                {
                    majorMultiplier = 1.0f;
                    break;
                }
            }

            // is this skill in the same Specialization as the class?
            if (skill.mData.mSpecialization == npcClass->mData.mSpecialization)
            {
                specMultiplier = 0.5f;
                specBonus = 5;
            }

            npcStats.getSkill(skill.mId).setBase(
                std::min(round_ieee_754(npcStats.getSkill(skill.mId).getBase() + 5 + raceBonus + specBonus
                             + (int(level) - 1) * (majorMultiplier + specMultiplier)),
                    100)); // Must gracefully handle level 0
        }

        if (!spellsInitialised)
        {
            std::vector<ESM::RefId> spells
                = MWMechanics::autoCalcNpcSpells(npcStats.getSkills(), npcStats.getAttributes(), race);
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
            MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
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

                for (size_t i = 0; i < ref->mBase->mNpdt.mSkills.size(); ++i)
                    data->mNpcStats.getSkill(ESM::Skill::indexToRefId(i)).setBase(ref->mBase->mNpdt.mSkills[i]);

                for (size_t i = 0; i < ref->mBase->mNpdt.mAttributes.size(); ++i)
                    data->mNpcStats.setAttribute(ESM::Attribute::indexToRefId(i), ref->mBase->mNpdt.mAttributes[i]);

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
            MWWorld::InventoryStore& inventory = getInventoryStore(ptr);
            inventory.setPtr(ptr);
            inventory.fill(ref->mBase->mInventory, ptr.getCellRef().getRefId(), prng);
            inventory.autoEquip();
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

    std::string_view Npc::getModel(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();
        std::string_view model = Settings::models().mBaseanim.get();
        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(ref->mBase->mRace);
        if (race->mData.mFlags & ESM::Race::Beast)
            model = Settings::models().mBaseanimkna.get();
        // Base animations should be in the meshes dir
        constexpr std::string_view prefix = "meshes/";
        assert(VFS::Path::pathEqual(prefix, model.substr(0, prefix.size())));
        return model.substr(prefix.size());
    }

    VFS::Path::Normalized Npc::getCorrectedModel(const MWWorld::ConstPtr& ptr) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(ref->mBase->mRace);
        if (race->mData.mFlags & ESM::Race::Beast)
            return Settings::models().mBaseanimkna.get();

        return Settings::models().mBaseanim.get();
    }

    void Npc::getModelsToPreload(const MWWorld::ConstPtr& ptr, std::vector<std::string_view>& models) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* npc = ptr.get<ESM::NPC>();
        const auto& esmStore = MWBase::Environment::get().getESMStore();
        models.push_back(getModel(ptr));

        if (!npc->mBase->mModel.empty())
            models.push_back(npc->mBase->mModel);

        if (!npc->mBase->mHead.empty())
        {
            const ESM::BodyPart* head = esmStore->get<ESM::BodyPart>().search(npc->mBase->mHead);
            if (head)
                models.push_back(head->mModel);
        }
        if (!npc->mBase->mHair.empty())
        {
            const ESM::BodyPart* hair = esmStore->get<ESM::BodyPart>().search(npc->mBase->mHair);
            if (hair)
                models.push_back(hair->mModel);
        }

        bool female = (npc->mBase->mFlags & ESM::NPC::Female);

        const MWWorld::CustomData* customData = ptr.getRefData().getCustomData();
        if (customData)
        {
            const MWWorld::InventoryStore& invStore = customData->asNpcCustomData().mInventoryStore;
            for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
            {
                MWWorld::ConstContainerStoreIterator equipped = invStore.getSlot(slot);
                if (equipped != invStore.end())
                {
                    const auto addParts = [&](const std::vector<ESM::PartReference>& parts) {
                        for (const ESM::PartReference& partRef : parts)
                        {
                            const ESM::RefId& partname
                                = (female && !partRef.mFemale.empty()) || (!female && partRef.mMale.empty())
                                ? partRef.mFemale
                                : partRef.mMale;

                            const ESM::BodyPart* part = esmStore->get<ESM::BodyPart>().search(partname);
                            if (part && !part->mModel.empty())
                                models.push_back(part->mModel);
                        }
                    };
                    if (equipped->getType() == ESM::Clothing::sRecordId)
                    {
                        const ESM::Clothing* clothes = equipped->get<ESM::Clothing>()->mBase;
                        addParts(clothes->mParts.mParts);
                    }
                    else if (equipped->getType() == ESM::Armor::sRecordId)
                    {
                        const ESM::Armor* armor = equipped->get<ESM::Armor>()->mBase;
                        addParts(armor->mParts.mParts);
                    }
                    else
                    {
                        std::string_view model = equipped->getClass().getModel(*equipped);
                        if (!model.empty())
                            models.push_back(model);
                    }
                }
            }
        }

        // preload body parts
        if (const ESM::Race* race = esmStore->get<ESM::Race>().search(npc->mBase->mRace))
        {
            const std::vector<const ESM::BodyPart*>& parts
                = MWRender::NpcAnimation::getBodyParts(race->mId, female, false, false);
            for (const ESM::BodyPart* part : parts)
            {
                if (part && !part->mModel.empty())
                    models.push_back(part->mModel);
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

        return getNameOrId<ESM::NPC>(ptr);
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

        const float dist = MWMechanics::getMeleeWeaponReach(ptr, weapon);
        const std::pair<MWWorld::Ptr, osg::Vec3f> result = MWMechanics::getHitContact(ptr, dist);
        if (result.first.isEmpty()) // Didn't hit anything
            return true;

        // Note that earlier we returned true in spite of an apparent failure to hit anything alive.
        // This is because hitting nothing is not a "miss" and should be handled as such character controller-side.
        victim = result.first;
        hitPosition = result.second;

        ESM::RefId weapskill = ESM::Skill::HandToHand;
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

        if (!MWMechanics::isInMeleeReach(ptr, victim, MWMechanics::getMeleeWeaponReach(ptr, weapon)))
            return;

        if (ptr == MWMechanics::getPlayer())
            MWBase::Environment::get().getWindowManager()->setEnemy(victim);

        float damage = 0.0f;
        if (!success)
        {
            MWBase::Environment::get().getLuaManager()->onHit(ptr, victim, weapon, MWWorld::Ptr(), type, attackStrength,
                damage, false, hitPosition, false, MWMechanics::DamageSourceType::Melee);
            MWMechanics::reduceWeaponCondition(damage, false, weapon, ptr);
            MWMechanics::resistNormalWeapon(victim, ptr, weapon, damage);
            return;
        }

        bool healthdmg;
        if (!weapon.isEmpty())
        {
            const unsigned char* attack = nullptr;
            if (type == ESM::Weapon::AT_Chop)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop.data();
            else if (type == ESM::Weapon::AT_Slash)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mSlash.data();
            else if (type == ESM::Weapon::AT_Thrust)
                attack = weapon.get<ESM::Weapon>()->mBase->mData.mThrust.data();
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
            ESM::RefId weapskill = ESM::Skill::HandToHand;
            if (!weapon.isEmpty())
                weapskill = weapon.getClass().getEquipmentSkill(weapon);
            skillUsageSucceeded(ptr, weapskill, ESM::Skill::Weapon_SuccessfulHit);

            const MWMechanics::AiSequence& seq = victim.getClass().getCreatureStats(victim).getAiSequence();

            bool unaware
                = !seq.isInCombat() && !MWBase::Environment::get().getMechanicsManager()->awarenessCheck(ptr, victim);
            if (unaware)
            {
                damage *= store.find("fCombatCriticalStrikeMult")->mValue.getFloat();
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTargetCriticalStrike}");
                if (healthdmg)
                {
                    MWBase::Environment::get().getSoundManager()->playSound3D(
                        victim, ESM::RefId::stringRefId("critical damage"), 1.0f, 1.0f);
                }
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

        MWBase::Environment::get().getLuaManager()->onHit(ptr, victim, weapon, MWWorld::Ptr(), type, attackStrength,
            damage, healthdmg, hitPosition, true, MWMechanics::DamageSourceType::Melee);
    }

    void Npc::onHit(const MWWorld::Ptr& ptr, const std::map<std::string, float>& damages, ESM::RefId object,
        const MWWorld::Ptr& attacker, bool successful, const MWMechanics::DamageSourceType sourceType) const
    {
        MWMechanics::CreatureStats& stats = getCreatureStats(ptr);
        bool wasDead = stats.isDead();

        bool setOnPcHitMe = true;

        // NOTE: 'object' and/or 'attacker' may be empty.
        if (!attacker.isEmpty() && attacker.getClass().isActor() && !stats.getAiSequence().isInCombat(attacker))
        {
            stats.setAttacked(true);
            bool complain = sourceType == MWMechanics::DamageSourceType::Melee;
            bool supportFriendlyFire = sourceType != MWMechanics::DamageSourceType::Ranged;
            if (supportFriendlyFire && MWMechanics::friendlyHit(attacker, ptr, complain))
                setOnPcHitMe = false;
            else
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

        if (!object.empty())
            stats.setLastHitAttemptObject(object);

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
            return;
        }

        if (!object.empty())
            stats.setLastHitObject(object);

        if (ptr == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState())
            return;

        bool hasDamage = false;
        bool hasHealthDamage = false;
        float healthDamage = 0.f;
        for (auto& [stat, damage] : damages)
        {
            if (damage < 0.001f)
                continue;
            hasDamage = true;

            if (stat == "health")
            {
                hasHealthDamage = true;
                healthDamage = damage;
                MWMechanics::DynamicStat<float> health(getCreatureStats(ptr).getHealth());
                health.setCurrent(health.getCurrent() - damage);
                stats.setHealth(health);
            }
            else if (stat == "fatigue")
            {
                MWMechanics::DynamicStat<float> fatigue(getCreatureStats(ptr).getFatigue());
                fatigue.setCurrent(fatigue.getCurrent() - damage, true);
                stats.setFatigue(fatigue);
            }
            else if (stat == "magicka")
            {
                MWMechanics::DynamicStat<float> magicka(getCreatureStats(ptr).getMagicka());
                magicka.setCurrent(magicka.getCurrent() - damage);
                stats.setMagicka(magicka);
            }
        }

        if (hasDamage && !attacker.isEmpty())
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
            if (hasHealthDamage && agilityTerm <= healthDamage && knockdownTerm <= Misc::Rng::roll0to99(prng))
                stats.setKnockedDown(true);
            else
                stats.setHitRecovery(true); // Is this supposed to always occur?
        }

        if (hasHealthDamage && healthDamage > 0.0f)
        {
            if (ptr == MWMechanics::getPlayer())
                MWBase::Environment::get().getWindowManager()->activateHitOverlay();
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
        const MWMechanics::AiSequence& aiSequence = stats.getAiSequence();
        const bool isPursuing = aiSequence.isInPursuit() && actor == MWMechanics::getPlayer();
        const bool inCombatWithActor = aiSequence.isInCombat(actor) || isPursuing;

        if (stats.isDead())
        {
            // by default user can loot non-fighting actors during death animation
            if (Settings::game().mCanLootDuringDeathAnimation)
                return std::make_unique<MWWorld::ActionOpen>(ptr);

            // otherwise wait until death animation
            if (stats.isDeathAnimationFinished())
                return std::make_unique<MWWorld::ActionOpen>(ptr);
        }
        else
        {
            const bool allowStealingFromKO
                = Settings::game().mAlwaysAllowStealingFromKnockedOutActors || !inCombatWithActor;
            if (stats.getKnockedDown() && allowStealingFromKO)
                return std::make_unique<MWWorld::ActionOpen>(ptr);

            const bool allowStealingWhileSneaking = !inCombatWithActor;
            if (MWBase::Environment::get().getMechanicsManager()->isSneaking(actor) && allowStealingWhileSneaking)
                return std::make_unique<MWWorld::ActionOpen>(ptr);

            const bool allowTalking = !inCombatWithActor && !getNpcStats(ptr).isWerewolf();
            if (allowTalking)
                return std::make_unique<MWWorld::ActionTalk>(ptr);
        }

        if (inCombatWithActor)
            return std::make_unique<MWWorld::FailedAction>("#{sActorInCombat}");

        return std::make_unique<MWWorld::FailedAction>();
    }

    MWWorld::ContainerStore& Npc::getContainerStore(const MWWorld::Ptr& ptr) const
    {
        return getInventoryStore(ptr);
    }

    MWWorld::InventoryStore& Npc::getInventoryStore(const MWWorld::Ptr& ptr) const
    {
        ensureCustomData(ptr);
        return ptr.getRefData().getCustomData()->asNpcCustomData().mInventoryStore;
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
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead())
            return 0.f;

        const MWBase::World* world = MWBase::Environment::get().getWorld();
        const GMST& gmst = getGmst();

        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();

        const float normalizedEncumbrance = getNormalizedEncumbrance(ptr);
        const bool running = MWBase::Environment::get().getMechanicsManager()->isRunning(ptr);

        float moveSpeed;
        if (normalizedEncumbrance > 1.0f)
            moveSpeed = 0.0f;
        else if (mageffects.getOrDefault(ESM::MagicEffect::Levitate).getMagnitude() > 0 && world->isLevitationEnabled())
        {
            float flySpeed = 0.01f
                * (stats.getAttribute(ESM::Attribute::Speed).getModified()
                    + mageffects.getOrDefault(ESM::MagicEffect::Levitate).getMagnitude());
            flySpeed = gmst.fMinFlySpeed->mValue.getFloat()
                + flySpeed * (gmst.fMaxFlySpeed->mValue.getFloat() - gmst.fMinFlySpeed->mValue.getFloat());
            flySpeed *= 1.0f - gmst.fEncumberedMoveEffect->mValue.getFloat() * normalizedEncumbrance;
            flySpeed = std::max(0.0f, flySpeed);
            moveSpeed = flySpeed;
        }
        else if (world->isSwimming(ptr))
            moveSpeed = getSwimSpeed(ptr);
        else if (running && !MWBase::Environment::get().getMechanicsManager()->isSneaking(ptr))
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
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead())
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
        x += mageffects.getOrDefault(ESM::MagicEffect::Jump).getMagnitude() * 64;
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

        const MWMechanics::AiSequence& aiSeq = customData.mNpcStats.getAiSequence();
        if (!aiSeq.isInCombat() || aiSeq.isFleeing())
            return true;

        if (Settings::game().mAlwaysAllowStealingFromKnockedOutActors && customData.mNpcStats.getKnockedDown())
            return true;

        return false;
    }

    MWGui::ToolTipInfo Npc::getToolTipInfo(const MWWorld::ConstPtr& ptr, int count) const
    {
        const MWWorld::LiveCellRef<ESM::NPC>* ref = ptr.get<ESM::NPC>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        std::string_view name = getName(ptr);
        info.caption = MyGUI::TextIterator::toTagsString(MyGUI::UString(name));
        if (fullHelp && !ref->mBase->mName.empty() && ptr.getRefData().getCustomData()
            && ptr.getRefData().getCustomData()->asNpcCustomData().mNpcStats.isWerewolf())
        {
            info.caption += " (";
            info.caption += MyGUI::TextIterator::toTagsString(ref->mBase->mName);
            info.caption += ")";
        }

        if (fullHelp)
            info.extra = MWGui::ToolTips::getMiscString(ref->mBase->mScript.getRefIdString(), "Script");

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

    void Npc::skillUsageSucceeded(const MWWorld::Ptr& ptr, ESM::RefId skill, int usageType, float extraFactor) const
    {
        MWBase::Environment::get().getLuaManager()->skillUse(ptr, skill, usageType, extraFactor);
    }

    float Npc::getArmorRating(const MWWorld::Ptr& ptr, bool useLuaInterfaceIfAvailable) const
    {
        if (useLuaInterfaceIfAvailable && ptr == MWMechanics::getPlayer())
        {
            auto res = MWLua::LocalScripts::callPlayerInterface<float>("Combat", "getArmorRating");
            if (res)
                return res.value();
        }

        // Fallback to the old engine implementation when actors don't have their scripts attached yet.

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
                ratings[i] = it->getClass().getSkillAdjustedArmorRating(*it, ptr);

                // Take in account armor condition
                const bool hasHealth = it->getClass().hasItemHealth(*it);
                if (hasHealth)
                {
                    ratings[i] *= it->getClass().getItemNormalizedHealth(*it);
                }
            }
        }

        float shield = stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Shield).getMagnitude();

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
                scale *= race->mData.mMaleHeight;
            else
                scale *= race->mData.mFemaleHeight;

            return;
        }

        if (ref->mBase->isMale())
        {
            scale.x() *= race->mData.mMaleWeight;
            scale.y() *= race->mData.mMaleWeight;
            scale.z() *= race->mData.mMaleHeight;
        }
        else
        {
            scale.x() *= race->mData.mFemaleWeight;
            scale.y() *= race->mData.mFemaleWeight;
            scale.z() *= race->mData.mFemaleHeight;
        }
    }

    int Npc::getServices(const MWWorld::ConstPtr& actor) const
    {
        const ESM::NPC* npc = actor.get<ESM::NPC>()->mBase;
        if (npc->mFlags & ESM::NPC::Autocalc)
        {
            const ESM::Class* npcClass = MWBase::Environment::get().getESMStore()->get<ESM::Class>().find(npc->mClass);
            return npcClass->mData.mServices;
        }
        return npc->mAiData.mServices;
    }

    ESM::RefId Npc::getSoundIdFromSndGen(const MWWorld::Ptr& ptr, std::string_view name) const
    {
        if (name == "left" || name == "right")
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            if (world->isFlying(ptr))
                return ESM::RefId();
            osg::Vec3f pos(ptr.getRefData().getPosition().asVec3());
            if (world->isSwimming(ptr))
                return (name == "left") ? npcParts.mSwimLeft : npcParts.mSwimRight;
            if (world->isUnderwater(ptr.getCell(), pos) || world->isWalkingOnWater(ptr))
                return (name == "left") ? npcParts.mFootWaterLeft : npcParts.mFootWaterRight;
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
                    return (name == "left") ? npcParts.mFootBareLeft : npcParts.mFootBareRight;

                ESM::RefId skill = boots->getClass().getEquipmentSkill(*boots);
                if (skill == ESM::Skill::LightArmor)
                    return (name == "left") ? npcParts.mFootLightLeft : npcParts.mFootLightRight;
                else if (skill == ESM::Skill::MediumArmor)
                    return (name == "left") ? npcParts.mFootMediumLeft : npcParts.mFootMediumRight;
                else if (skill == ESM::Skill::HeavyArmor)
                    return (name == "left") ? npcParts.mFootHeavyLeft : npcParts.mFootHeavyRight;
            }
            return ESM::RefId();
        }

        // Morrowind ignores land soundgen for NPCs
        if (name == "land")
            return ESM::RefId();
        if (name == "swimleft")
            return npcParts.mSwimLeft;
        if (name == "swimright")
            return npcParts.mSwimRight;
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
        MWWorld::Ptr newPtr(cell.insert(ref), &cell);
        if (newPtr.getRefData().getCustomData())
        {
            MWBase::Environment::get().getWorldModel()->registerPtr(newPtr);
            newPtr.getClass().getContainerStore(newPtr).setPtr(newPtr);
        }
        return newPtr;
    }

    float Npc::getSkill(const MWWorld::Ptr& ptr, ESM::RefId id) const
    {
        return getNpcStats(ptr).getSkill(id).getModified();
    }

    void Npc::readAdditionalState(const MWWorld::Ptr& ptr, const ESM::ObjectState& state) const
    {
        if (!state.mHasCustomState)
            return;

        const ESM::NpcState& npcState = state.asNpcState();

        if (!ptr.getRefData().getCustomData())
        {
            if (npcState.mCreatureStats.mMissingACDT)
                ensureCustomData(ptr);
            else
            {
                // Create a CustomData, but don't fill it from ESM records (not needed)
                auto data = std::make_unique<NpcCustomData>();
                MWBase::Environment::get().getWorldModel()->registerPtr(ptr);
                data->mInventoryStore.setPtr(ptr);
                ptr.getRefData().setCustomData(std::move(data));
            }
        }

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
        if (ptr.getCellRef().getCount() <= 0
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
        if (ptr.getCellRef().getCount() > 0 && !creatureStats.isDead())
            return;

        if (!creatureStats.isDeathAnimationFinished())
            return;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fCorpseRespawnDelay = gmst.find("fCorpseRespawnDelay")->mValue.getFloat();
        static const float fCorpseClearDelay = gmst.find("fCorpseClearDelay")->mValue.getFloat();

        float delay
            = ptr.getCellRef().getCount() == 0 ? fCorpseClearDelay : std::min(fCorpseRespawnDelay, fCorpseClearDelay);

        if (ptr.get<ESM::NPC>()->mBase->mFlags & ESM::NPC::Respawn
            && creatureStats.getTimeOfDeath() + delay <= MWBase::Environment::get().getWorld()->getTimeStamp())
        {
            if (ptr.getCellRef().hasContentFile())
            {
                if (ptr.getCellRef().getCount() == 0)
                {
                    ptr.getCellRef().setCount(1);
                    const ESM::RefId& script = getScript(ptr);
                    if (!script.empty())
                        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, ptr);
                }

                MWBase::Environment::get().getWorld()->removeContainerScripts(ptr);
                MWBase::Environment::get().getWindowManager()->onDeleteCustomData(ptr);
                ptr.getRefData().setCustomData(nullptr);

                // Reset to original position
                MWBase::Environment::get().getWorld()->moveObject(
                    ptr, ptr.getCell()->getOriginCell(ptr), ptr.getCellRef().getPosition().asVec3());
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
        const MWMechanics::MagicEffects& effects = getNpcStats(ptr).getMagicEffects();
        const bool running = MWBase::Environment::get().getMechanicsManager()->isRunning(ptr);
        return getSwimSpeedImpl(ptr, getGmst(), effects, running ? getRunSpeed(ptr) : getWalkSpeed(ptr));
    }
}
