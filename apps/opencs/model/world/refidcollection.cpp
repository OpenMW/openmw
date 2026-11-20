#include "refidcollection.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#include <apps/opencs/model/world/columnbase.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/refiddata.hpp>

#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadlock.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadprob.hpp>
#include <components/esm3/loadrepa.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/loadweap.hpp>

#include "columns.hpp"
#include "nestedcoladapterimp.hpp"
#include "nestedtablewrapper.hpp"
#include "refidadapter.hpp"
#include "refidadapterimp.hpp"
#include "variantconversion.hpp"

CSMWorld::RefIdColumn::RefIdColumn(int columnId, Display displayType, int flag, bool editable, bool userEditable)
    : NestableColumn(columnId, displayType, flag)
    , mEditable(editable)
    , mUserEditable(userEditable)
{
}

bool CSMWorld::RefIdColumn::isEditable() const
{
    return mEditable;
}

bool CSMWorld::RefIdColumn::isUserEditable() const
{
    return mUserEditable;
}

const CSMWorld::RefIdAdapter& CSMWorld::RefIdCollection::findAdapter(UniversalId::Type type) const
{
    const auto iter = mAdapters.find(type);

    if (iter == mAdapters.end())
        throw std::logic_error("unsupported type in RefIdCollection");

    return *iter->second;
}

CSMWorld::RefIdCollection::RefIdCollection(const IdCollection<ESM::MagicEffect>& magicEffects)
{
    using enum Columns::ColumnId;
    using enum ColumnBase::Display;
    using enum ColumnBase::Flags;

    BaseColumns baseColumns;

    baseColumns.mId = addColumn(ColumnId_Id, Display_Id, Flag_Table | Flag_Dialogue, false, false);
    baseColumns.mModified
        = addColumn(ColumnId_Modification, Display_RecordState, Flag_Table | Flag_Dialogue, true, false);
    baseColumns.mType = addColumn(ColumnId_RecordType, Display_RefRecordType, Flag_Table | Flag_Dialogue, false, false);
    baseColumns.mBlocked
        = addColumn(ColumnId_Blocked, Display_Boolean, Flag_Table | Flag_Dialogue | Flag_Dialogue_Refresh);

    ModelColumns modelColumns(baseColumns);

    modelColumns.mPersistence = addColumn(ColumnId_Persistent, Display_Boolean);
    modelColumns.mModel = addColumn(ColumnId_Model, Display_Mesh);

    NameColumns nameColumns(modelColumns);

    // Only items that can be placed in a container have the 32 character limit, but enforce
    // that for all referenceable types for now.
    nameColumns.mName = addColumn(ColumnId_Name, Display_String32);
    nameColumns.mScript = addColumn(ColumnId_Script, Display_Script);

    InventoryColumns inventoryColumns(nameColumns);

    inventoryColumns.mIcon = addColumn(ColumnId_Icon, Display_Icon);
    inventoryColumns.mWeight = addColumn(ColumnId_Weight, Display_Float);
    inventoryColumns.mValue = addColumn(ColumnId_GoldValue, Display_Integer);

    IngredientColumns ingredientColumns(inventoryColumns);
    ingredientColumns.mEffects = addColumn(ColumnId_EffectList, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> ingredientEffectsMap;
    ingredientEffectsMap.insert(
        std::make_pair(UniversalId::Type_Ingredient, new IngredEffectRefIdAdapter(magicEffects)));
    mNestedAdapters.emplace_back(&mColumns.back(), ingredientEffectsMap);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_EffectId, Display_IngredEffectId);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_Skill, Display_EffectSkill);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_Attribute, Display_EffectAttribute);

    // nested table
    PotionColumns potionColumns(inventoryColumns);
    potionColumns.mEffects = addColumn(ColumnId_EffectList, Display_NestedHeader,
        Flag_Dialogue); // see refidadapterimp.hpp
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> effectsMap;
    effectsMap.insert(std::make_pair(
        UniversalId::Type_Potion, new EffectsRefIdAdapter<ESM::Potion>(UniversalId::Type_Potion, magicEffects)));
    mNestedAdapters.emplace_back(&mColumns.back(), effectsMap);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_EffectId, Display_EffectId);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_Skill, Display_EffectSkill);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_Attribute, Display_EffectAttribute);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_EffectRange, Display_EffectRange);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_EffectArea, Display_String);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_Duration, Display_Integer); // reuse from light
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_MinMagnitude, Display_Integer);
    mColumns.back().addColumn<NestedChildColumn>(ColumnId_MaxMagnitude, Display_Integer);

    EnchantableColumns enchantableColumns(inventoryColumns);

    enchantableColumns.mEnchantment = addColumn(ColumnId_Enchantment, Display_Enchantment);
    enchantableColumns.mEnchantmentPoints = addColumn(ColumnId_EnchantmentPoints, Display_Integer);

    ToolColumns toolsColumns(inventoryColumns);

    toolsColumns.mQuality = addColumn(ColumnId_Quality, Display_Float);
    toolsColumns.mUses = addColumn(ColumnId_Charges, Display_Integer);

    ActorColumns actorsColumns(nameColumns);

    actorsColumns.mHello = addColumn(ColumnId_AiHello, Display_UnsignedInteger16);
    actorsColumns.mFlee = addColumn(ColumnId_AiFlee, Display_UnsignedInteger8);
    actorsColumns.mFight = addColumn(ColumnId_AiFight, Display_UnsignedInteger8);
    actorsColumns.mAlarm = addColumn(ColumnId_AiAlarm, Display_UnsignedInteger8);

    // Nested table
    actorsColumns.mInventory = addColumn(ColumnId_ActorInventory, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> inventoryMap;
    inventoryMap.insert(
        std::make_pair(UniversalId::Type_Npc, new NestedInventoryRefIdAdapter<ESM::NPC>(UniversalId::Type_Npc)));
    inventoryMap.insert(std::make_pair(
        UniversalId::Type_Creature, new NestedInventoryRefIdAdapter<ESM::Creature>(UniversalId::Type_Creature)));
    mNestedAdapters.emplace_back(&mColumns.back(), inventoryMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_InventoryItemId, Display_Referenceable);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_ItemCount, Display_Integer);

    // Nested table
    actorsColumns.mSpells = addColumn(ColumnId_SpellList, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> spellsMap;
    spellsMap.insert(
        std::make_pair(UniversalId::Type_Npc, new NestedSpellRefIdAdapter<ESM::NPC>(UniversalId::Type_Npc)));
    spellsMap.insert(std::make_pair(
        UniversalId::Type_Creature, new NestedSpellRefIdAdapter<ESM::Creature>(UniversalId::Type_Creature)));
    mNestedAdapters.emplace_back(&mColumns.back(), spellsMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_SpellId, Display_Spell);

    // Nested table
    actorsColumns.mDestinations = addColumn(ColumnId_NpcDestinations, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> destMap;
    destMap.insert(
        std::make_pair(UniversalId::Type_Npc, new NestedTravelRefIdAdapter<ESM::NPC>(UniversalId::Type_Npc)));
    destMap.insert(std::make_pair(
        UniversalId::Type_Creature, new NestedTravelRefIdAdapter<ESM::Creature>(UniversalId::Type_Creature)));
    mNestedAdapters.emplace_back(&mColumns.back(), destMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_DestinationCell, Display_Cell);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PosX, Display_Float);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PosY, Display_Float);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PosZ, Display_Float);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_RotX, Display_Double);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_RotY, Display_Double);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_RotZ, Display_Double);

    // Nested table
    actorsColumns.mAiPackages = addColumn(ColumnId_AiPackageList, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> aiMap;
    aiMap.insert(std::make_pair(UniversalId::Type_Npc, new ActorAiRefIdAdapter<ESM::NPC>(UniversalId::Type_Npc)));
    aiMap.insert(
        std::make_pair(UniversalId::Type_Creature, new ActorAiRefIdAdapter<ESM::Creature>(UniversalId::Type_Creature)));
    mNestedAdapters.emplace_back(&mColumns.back(), aiMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiPackageType, Display_AiPackageType);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiWanderDist, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiDuration, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiWanderToD, Display_Integer);

    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle2, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle3, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle4, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle5, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle6, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle7, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle8, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Idle9, Display_Integer);

    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiWanderRepeat, Display_Boolean);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiActivateName, Display_String32);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiTargetId, Display_String32);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AiTargetCell, Display_String);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PosX, Display_Float);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PosY, Display_Float);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PosZ, Display_Float);

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sServiceTable[] = { { ColumnId_BuysWeapons, ESM::NPC::Weapon }, { ColumnId_BuysArmor, ESM::NPC::Armor },
        { ColumnId_BuysClothing, ESM::NPC::Clothing }, { ColumnId_BuysBooks, ESM::NPC::Books },
        { ColumnId_BuysIngredients, ESM::NPC::Ingredients }, { ColumnId_BuysLockpicks, ESM::NPC::Picks },
        { ColumnId_BuysProbes, ESM::NPC::Probes }, { ColumnId_BuysLights, ESM::NPC::Lights },
        { ColumnId_BuysApparati, ESM::NPC::Apparatus }, { ColumnId_BuysRepairItems, ESM::NPC::RepairItem },
        { ColumnId_BuysMiscItems, ESM::NPC::Misc }, { ColumnId_BuysPotions, ESM::NPC::Potions },
        { ColumnId_BuysMagicItems, ESM::NPC::MagicItems }, { ColumnId_SellsSpells, ESM::NPC::Spells },
        { ColumnId_Trainer, ESM::NPC::Training }, { ColumnId_Spellmaking, ESM::NPC::Spellmaking },
        { ColumnId_EnchantingService, ESM::NPC::Enchanting }, { ColumnId_RepairService, ESM::NPC::Repair }, { -1, 0 } };

    for (int i = 0; sServiceTable[i].mName != -1; ++i)
    {
        mColumns.emplace_back(sServiceTable[i].mName, Display_Boolean);
        actorsColumns.mServices.insert(std::make_pair(&mColumns.back(), sServiceTable[i].mFlag));
    }

    const auto* autoCalc
        = addColumn(ColumnId_AutoCalc, Display_Boolean, Flag_Table | Flag_Dialogue | Flag_Dialogue_Refresh);

    const auto* apparatusType = addColumn(ColumnId_ApparatusType, Display_ApparatusType);
    const auto* armorType = addColumn(ColumnId_ArmorType, Display_ArmorType);
    const auto* health = addColumn(ColumnId_Health, Display_Integer);
    const auto* armor = addColumn(ColumnId_ArmorValue, Display_Integer);
    const auto* bookType = addColumn(ColumnId_BookType, Display_BookType);
    const auto* skill = addColumn(ColumnId_Skill, Display_SkillId);
    const auto* text = addColumn(ColumnId_Text, Display_LongString);
    const auto* clothingType = addColumn(ColumnId_ClothingType, Display_ClothingType);
    const auto* weightCapacity = addColumn(ColumnId_WeightCapacity, Display_Float);
    const auto* organic = addColumn(ColumnId_OrganicContainer, Display_Boolean);
    const auto* respawn = addColumn(ColumnId_Respawn, Display_Boolean);

    // Nested table
    const auto* content = addColumn(ColumnId_ContainerContent, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> contMap;
    contMap.insert(std::make_pair(
        UniversalId::Type_Container, new NestedInventoryRefIdAdapter<ESM::Container>(UniversalId::Type_Container)));
    mNestedAdapters.emplace_back(&mColumns.back(), contMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_InventoryItemId, Display_Referenceable);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_ItemCount, Display_Integer);

    CreatureColumns creatureColumns(actorsColumns);

    creatureColumns.mType = addColumn(ColumnId_CreatureType, Display_CreatureType);
    creatureColumns.mScale = addColumn(ColumnId_Scale, Display_Float);
    creatureColumns.mOriginal = addColumn(ColumnId_ParentCreature, Display_Creature);

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sCreatureFlagTable[]
        = { { ColumnId_Biped, ESM::Creature::Bipedal }, { ColumnId_HasWeapon, ESM::Creature::Weapon },
              { ColumnId_Swims, ESM::Creature::Swims }, { ColumnId_Flies, ESM::Creature::Flies },
              { ColumnId_Walks, ESM::Creature::Walks }, { ColumnId_Essential, ESM::Creature::Essential }, { -1, 0 } };

    // for re-use in NPC records
    const RefIdColumn* essential = nullptr;

    for (int i = 0; sCreatureFlagTable[i].mName != -1; ++i)
    {
        mColumns.emplace_back(sCreatureFlagTable[i].mName, Display_Boolean);
        creatureColumns.mFlags.insert(std::make_pair(&mColumns.back(), sCreatureFlagTable[i].mFlag));

        switch (sCreatureFlagTable[i].mFlag)
        {
            case ESM::Creature::Essential:
                essential = &mColumns.back();
                break;
        }
    }

    mColumns.emplace_back(ColumnId_BloodType, Display_BloodType);
    // For re-use in NPC records.
    const RefIdColumn* bloodType = &mColumns.back();
    creatureColumns.mBloodType = bloodType;

    creatureColumns.mFlags.insert(std::make_pair(respawn, ESM::Creature::Respawn));

    // Nested table
    creatureColumns.mAttributes = addColumn(ColumnId_CreatureAttributes, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> creaAttrMap;
    creaAttrMap.insert(std::make_pair(UniversalId::Type_Creature, new CreatureAttributesRefIdAdapter()));
    mNestedAdapters.emplace_back(&mColumns.back(), creaAttrMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Attribute, Display_Attribute, false, false);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_AttributeValue, Display_Integer);

    // Nested table
    creatureColumns.mAttacks = addColumn(ColumnId_CreatureAttack, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> attackMap;
    attackMap.insert(std::make_pair(UniversalId::Type_Creature, new CreatureAttackRefIdAdapter()));
    mNestedAdapters.emplace_back(&mColumns.back(), attackMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_CreatureAttack, Display_Integer, false, false);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_MinAttack, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_MaxAttack, Display_Integer);

    // Nested list
    creatureColumns.mMisc = addColumn(ColumnId_CreatureMisc, Display_NestedHeader, Flag_Dialogue | Flag_Dialogue_List);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> creaMiscMap;
    creaMiscMap.insert(std::make_pair(UniversalId::Type_Creature, new CreatureMiscRefIdAdapter()));
    mNestedAdapters.emplace_back(&mColumns.back(), creaMiscMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Level, Display_Integer, Flag_Dialogue | Flag_Dialogue_Refresh);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Health, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Mana, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Fatigue, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_SoulPoints, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_CombatState, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_MagicState, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_StealthState, Display_Integer);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Gold, Display_Integer);

    const RefIdColumn* openSound = addColumn(ColumnId_OpenSound, Display_Sound);

    const RefIdColumn* closeSound = addColumn(ColumnId_CloseSound, Display_Sound);

    LightColumns lightColumns(inventoryColumns);

    lightColumns.mTime = addColumn(ColumnId_Duration, Display_Integer);

    lightColumns.mRadius = addColumn(ColumnId_Radius, Display_Integer);

    lightColumns.mColor = addColumn(ColumnId_Colour, Display_Colour);

    lightColumns.mSound = addColumn(ColumnId_Sound, Display_Sound);

    lightColumns.mEmitterType = addColumn(ColumnId_EmitterType, Display_EmitterType);

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sLightFlagTable[] = { { ColumnId_Dynamic, ESM::Light::Dynamic }, { ColumnId_Portable, ESM::Light::Carry },
        { ColumnId_NegativeLight, ESM::Light::Negative }, { ColumnId_Fire, ESM::Light::Fire },
        { ColumnId_OffByDefault, ESM::Light::OffDefault }, { -1, 0 } };

    for (int i = 0; sLightFlagTable[i].mName != -1; ++i)
    {
        mColumns.emplace_back(sLightFlagTable[i].mName, Display_Boolean);
        lightColumns.mFlags.insert(std::make_pair(&mColumns.back(), sLightFlagTable[i].mFlag));
    }

    const RefIdColumn* key = addColumn(ColumnId_IsKey, Display_Boolean);

    NpcColumns npcColumns(actorsColumns);

    npcColumns.mRace = addColumn(ColumnId_Race, Display_Race);

    npcColumns.mClass = addColumn(ColumnId_Class, Display_Class);

    // NAME32 enforced in IdCompletionDelegate::createEditor()
    npcColumns.mFaction = addColumn(ColumnId_Faction, Display_Faction);

    npcColumns.mHair = addColumn(Columns::Columnid_Hair, Display_BodyPart);

    npcColumns.mHead = addColumn(ColumnId_Head, Display_BodyPart);

    npcColumns.mGender = addColumn(ColumnId_Gender, Display_GenderNpc);

    npcColumns.mFlags.insert(std::make_pair(essential, ESM::NPC::Essential));

    npcColumns.mFlags.insert(std::make_pair(respawn, ESM::NPC::Respawn));

    npcColumns.mFlags.insert(std::make_pair(autoCalc, ESM::NPC::Autocalc));

    // Re-used from Creature records.
    npcColumns.mBloodType = bloodType;

    // Need a way to add a table of stats and values (rather than adding a long list of
    // entries in the dialogue subview) E.g. attributes+stats(health, mana, fatigue), skills
    // These needs to be driven from the autocalculated setting.

    // Nested table
    npcColumns.mAttributes = addColumn(ColumnId_NpcAttributes, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> attrMap;
    attrMap.insert(std::make_pair(UniversalId::Type_Npc, new NpcAttributesRefIdAdapter()));
    mNestedAdapters.emplace_back(&mColumns.back(), attrMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Attribute, Display_Attribute, false, false);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_UChar, Display_UnsignedInteger8);

    // Nested table
    npcColumns.mSkills = addColumn(ColumnId_NpcSkills, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> skillsMap;
    skillsMap.insert(std::make_pair(UniversalId::Type_Npc, new NpcSkillsRefIdAdapter()));
    mNestedAdapters.emplace_back(&mColumns.back(), skillsMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Skill, Display_SkillId, false, false);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_UChar, Display_UnsignedInteger8);

    // Nested list
    npcColumns.mMisc = addColumn(ColumnId_NpcMisc, Display_NestedHeader, Flag_Dialogue | Flag_Dialogue_List);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> miscMap;
    miscMap.insert(std::make_pair(UniversalId::Type_Npc, new NpcMiscRefIdAdapter()));
    mNestedAdapters.emplace_back(&mColumns.back(), miscMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Level, Display_SignedInteger16);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Health, Display_UnsignedInteger16);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Mana, Display_UnsignedInteger16);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Fatigue, Display_UnsignedInteger16);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_NpcDisposition, Display_UnsignedInteger8);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_NpcReputation, Display_UnsignedInteger8);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_NpcRank, Display_UnsignedInteger8);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_Gold, Display_Integer);

    WeaponColumns weaponColumns(enchantableColumns);

    weaponColumns.mType = addColumn(ColumnId_WeaponType, Display_WeaponType);

    weaponColumns.mHealth = health;

    weaponColumns.mSpeed = addColumn(ColumnId_WeaponSpeed, Display_Float);

    weaponColumns.mReach = addColumn(ColumnId_WeaponReach, Display_Float);

    for (int i = 0; i < 3; ++i)
    {
        const RefIdColumn** column
            = i == 0 ? weaponColumns.mChop : (i == 1 ? weaponColumns.mSlash : weaponColumns.mThrust);

        for (int j = 0; j < 2; ++j)
        {
            column[j] = addColumn(ColumnId_MinChop + i * 2 + j, Display_Integer);
        }
    }

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sWeaponFlagTable[]
        = { { ColumnId_Magical, ESM::Weapon::Magical }, { ColumnId_Silver, ESM::Weapon::Silver }, { -1, 0 } };

    for (int i = 0; sWeaponFlagTable[i].mName != -1; ++i)
    {
        mColumns.emplace_back(sWeaponFlagTable[i].mName, Display_Boolean);
        weaponColumns.mFlags.insert(std::make_pair(&mColumns.back(), sWeaponFlagTable[i].mFlag));
    }

    // Nested table
    const RefIdColumn* partRef = addColumn(ColumnId_PartRefList, Display_NestedHeader, Flag_Dialogue);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> partMap;
    partMap.insert(
        std::make_pair(UniversalId::Type_Armor, new BodyPartRefIdAdapter<ESM::Armor>(UniversalId::Type_Armor)));
    partMap.insert(std::make_pair(
        UniversalId::Type_Clothing, new BodyPartRefIdAdapter<ESM::Clothing>(UniversalId::Type_Clothing)));
    mNestedAdapters.emplace_back(&mColumns.back(), partMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PartRefType, Display_PartRefType);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PartRefMale, Display_BodyPart);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_PartRefFemale, Display_BodyPart);

    LevListColumns creatureLevListColumns(baseColumns);
    LevListColumns itemLevListColumns(baseColumns);
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> creatureLevListMap, itemLevListMap;
    creatureLevListMap.insert(std::make_pair(UniversalId::Type_CreatureLevelledList,
        new NestedLevListRefIdAdapter<ESM::CreatureLevList>(UniversalId::Type_CreatureLevelledList)));
    itemLevListMap.insert(std::make_pair(UniversalId::Type_ItemLevelledList,
        new NestedLevListRefIdAdapter<ESM::ItemLevList>(UniversalId::Type_ItemLevelledList)));

    // Levelled creature nested table
    creatureLevListColumns.mLevList = addColumn(ColumnId_LevelledList, Display_NestedHeader, Flag_Dialogue);
    mNestedAdapters.emplace_back(&mColumns.back(), creatureLevListMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledCreatureId, Display_Referenceable);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledItemLevel, Display_Integer);

    // Levelled item nested table
    itemLevListColumns.mLevList = addColumn(ColumnId_LevelledList, Display_NestedHeader, Flag_Dialogue);
    mNestedAdapters.emplace_back(&mColumns.back(), itemLevListMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledItemId, Display_Referenceable);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledItemLevel, Display_Integer);

    // Shared levelled list nested list
    creatureLevListColumns.mNestedListLevList
        = addColumn(ColumnId_LevelledList, Display_NestedHeader, Flag_Dialogue | Flag_Dialogue_List);
    itemLevListColumns.mNestedListLevList = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> nestedListLevListMap;
    nestedListLevListMap.insert(std::make_pair(UniversalId::Type_CreatureLevelledList,
        new NestedListLevListRefIdAdapter<ESM::CreatureLevList>(UniversalId::Type_CreatureLevelledList)));
    nestedListLevListMap.insert(std::make_pair(UniversalId::Type_ItemLevelledList,
        new NestedListLevListRefIdAdapter<ESM::ItemLevList>(UniversalId::Type_ItemLevelledList)));
    mNestedAdapters.emplace_back(&mColumns.back(), nestedListLevListMap);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledItemTypeEach, Display_Boolean);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledItemType, Display_Boolean);
    mColumns.back().addColumn<RefIdColumn>(ColumnId_LevelledItemChanceNone, Display_UnsignedInteger8);

    mAdapters.insert(std::make_pair(
        UniversalId::Type_Activator, new NameRefIdAdapter<ESM::Activator>(UniversalId::Type_Activator, nameColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Potion, new PotionRefIdAdapter(potionColumns, autoCalc)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Apparatus,
        new ApparatusRefIdAdapter(inventoryColumns, apparatusType, toolsColumns.mQuality)));
    mAdapters.insert(std::make_pair(
        UniversalId::Type_Armor, new ArmorRefIdAdapter(enchantableColumns, armorType, health, armor, partRef)));
    mAdapters.insert(
        std::make_pair(UniversalId::Type_Book, new BookRefIdAdapter(enchantableColumns, bookType, skill, text)));
    mAdapters.insert(std::make_pair(
        UniversalId::Type_Clothing, new ClothingRefIdAdapter(enchantableColumns, clothingType, partRef)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Container,
        new ContainerRefIdAdapter(nameColumns, weightCapacity, organic, respawn, content)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Creature, new CreatureRefIdAdapter(creatureColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Door, new DoorRefIdAdapter(nameColumns, openSound, closeSound)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Ingredient, new IngredientRefIdAdapter(ingredientColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_CreatureLevelledList,
        new LevelledListRefIdAdapter<ESM::CreatureLevList>(
            UniversalId::Type_CreatureLevelledList, creatureLevListColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_ItemLevelledList,
        new LevelledListRefIdAdapter<ESM::ItemLevList>(UniversalId::Type_ItemLevelledList, itemLevListColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Light, new LightRefIdAdapter(lightColumns)));
    mAdapters.insert(std::make_pair(
        UniversalId::Type_Lockpick, new ToolRefIdAdapter<ESM::Lockpick>(UniversalId::Type_Lockpick, toolsColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Miscellaneous, new MiscRefIdAdapter(inventoryColumns, key)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Npc, new NpcRefIdAdapter(npcColumns)));
    mAdapters.insert(std::make_pair(
        UniversalId::Type_Probe, new ToolRefIdAdapter<ESM::Probe>(UniversalId::Type_Probe, toolsColumns)));
    mAdapters.insert(std::make_pair(
        UniversalId::Type_Repair, new ToolRefIdAdapter<ESM::Repair>(UniversalId::Type_Repair, toolsColumns)));
    mAdapters.insert(std::make_pair(
        UniversalId::Type_Static, new ModelRefIdAdapter<ESM::Static>(UniversalId::Type_Static, modelColumns)));
    mAdapters.insert(std::make_pair(UniversalId::Type_Weapon, new WeaponRefIdAdapter(weaponColumns)));
}

CSMWorld::RefIdCollection::~RefIdCollection()
{
    for (auto iter(mAdapters.begin()); iter != mAdapters.end(); ++iter)
        delete iter->second;

    for (auto iter(mNestedAdapters.begin()); iter != mNestedAdapters.end(); ++iter)
    {
        for (auto it((iter->second).begin()); it != (iter->second).end(); ++it)
            delete it->second;
    }
}

int CSMWorld::RefIdCollection::getSize() const
{
    return mData.getSize();
}

ESM::RefId CSMWorld::RefIdCollection::getId(int index) const
{
    return getRefId(getData(index, 0));
}

int CSMWorld::RefIdCollection::getIndex(const ESM::RefId& id) const
{
    int index = searchId(id);

    if (index == -1)
        throw std::runtime_error("ID is not found in RefId collection: " + id.toDebugString());

    return index;
}

int CSMWorld::RefIdCollection::getColumns() const
{
    return static_cast<int>(mColumns.size());
}

const CSMWorld::ColumnBase& CSMWorld::RefIdCollection::getColumn(int column) const
{
    return mColumns.at(column);
}

QVariant CSMWorld::RefIdCollection::getData(int index, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(index);

    const RefIdAdapter& adaptor = findAdapter(localIndex.second);

    return adaptor.getData(&mColumns.at(column), mData, localIndex.first);
}

QVariant CSMWorld::RefIdCollection::getNestedData(int row, int column, int subRow, int subColumn) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.getNestedData(&mColumns.at(column), mData, localIndex.first, subRow, subColumn);
}

void CSMWorld::RefIdCollection::setData(int index, int column, const QVariant& data)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(index);

    const RefIdAdapter& adaptor = findAdapter(localIndex.second);

    adaptor.setData(&mColumns.at(column), mData, localIndex.first, data);
}

void CSMWorld::RefIdCollection::setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    nestedAdapter.setNestedData(&mColumns.at(column), mData, localIndex.first, data, subRow, subColumn);
}

void CSMWorld::RefIdCollection::removeRows(int index, int count)
{
    mData.erase(index, count);
}

void CSMWorld::RefIdCollection::removeNestedRows(int row, int column, int subRow)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    nestedAdapter.removeNestedRow(&mColumns.at(column), mData, localIndex.first, subRow);
}

void CSMWorld::RefIdCollection::appendBlankRecord(const ESM::RefId& id, UniversalId::Type type)
{
    mData.appendRecord(type, id, false);
}

int CSMWorld::RefIdCollection::searchId(const ESM::RefId& id) const
{
    const RefIdData::LocalIndex localIndex = mData.searchId(id);

    if (localIndex.first == -1)
        return -1;

    return mData.localToGlobalIndex(localIndex);
}

void CSMWorld::RefIdCollection::replace(int index, std::unique_ptr<RecordBase> record)
{
    mData.getRecord(mData.globalToLocalIndex(index)).assign(*record.release());
}

void CSMWorld::RefIdCollection::cloneRecord(
    const ESM::RefId& origin, const ESM::RefId& destination, const CSMWorld::UniversalId::Type type)
{
    std::unique_ptr<RecordBase> newRecord = mData.getRecord(mData.searchId(origin)).modifiedCopy();
    auto adapter = mAdapters.find(type);
    assert(adapter != mAdapters.end());
    adapter->second->setId(*newRecord, destination.getRefIdString());
    mData.insertRecord(std::move(newRecord), type, destination);
}

bool CSMWorld::RefIdCollection::touchRecord(const ESM::RefId& id)
{
    throw std::runtime_error("RefIdCollection::touchRecord is unimplemented");
}

void CSMWorld::RefIdCollection::appendRecord(std::unique_ptr<RecordBase> record, UniversalId::Type type)
{
    auto id = findAdapter(type).getId(*record.get());

    int index = mData.getAppendIndex(type);

    mData.appendRecord(type, id, false);

    mData.getRecord(mData.globalToLocalIndex(index)).assign(*record.release());
}

const CSMWorld::RecordBase& CSMWorld::RefIdCollection::getRecord(const ESM::RefId& id) const
{
    return mData.getRecord(mData.searchId(id));
}

const CSMWorld::RecordBase& CSMWorld::RefIdCollection::getRecord(int index) const
{
    return mData.getRecord(mData.globalToLocalIndex(index));
}

void CSMWorld::RefIdCollection::load(ESM::ESMReader& reader, bool base, UniversalId::Type type)
{
    mData.load(reader, base, type);
}

int CSMWorld::RefIdCollection::getAppendIndex(const ESM::RefId& id, UniversalId::Type type) const
{
    return mData.getAppendIndex(type);
}

std::vector<ESM::RefId> CSMWorld::RefIdCollection::getIds(bool listDeleted) const
{
    return mData.getIds(listDeleted);
}

bool CSMWorld::RefIdCollection::reorderRows(int baseIndex, const std::vector<int>& newOrder)
{
    return false;
}

void CSMWorld::RefIdCollection::save(int index, ESM::ESMWriter& writer) const
{
    mData.save(index, writer);
}

const CSMWorld::RefIdData& CSMWorld::RefIdCollection::getDataSet() const
{
    return mData;
}

int CSMWorld::RefIdCollection::getNestedRowsCount(int row, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.getNestedRowsCount(&mColumns.at(column), mData, localIndex.first);
}

int CSMWorld::RefIdCollection::getNestedColumnsCount(int row, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.getNestedColumnsCount(&mColumns.at(column), mData);
}

CSMWorld::NestableColumn* CSMWorld::RefIdCollection::getNestableColumn(int column)
{
    return &mColumns.at(column);
}

void CSMWorld::RefIdCollection::addNestedRow(int row, int col, int position)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(col), localIndex.second);

    nestedAdapter.addNestedRow(&mColumns.at(col), mData, localIndex.first, position);
}

void CSMWorld::RefIdCollection::setNestedTable(int row, int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    nestedAdapter.setNestedTable(&mColumns.at(column), mData, localIndex.first, nestedTable);
}

CSMWorld::NestedTableWrapperBase* CSMWorld::RefIdCollection::nestedTable(int row, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.nestedTable(&mColumns.at(column), mData, localIndex.first);
}

const CSMWorld::NestedRefIdAdapterBase& CSMWorld::RefIdCollection::getNestedAdapter(
    const CSMWorld::ColumnBase& column, UniversalId::Type type) const
{
    for (auto iter(mNestedAdapters.begin()); iter != mNestedAdapters.end(); ++iter)
    {
        if ((iter->first) == &column)
        {
            const auto it = (iter->second).find(type);

            if (it == (iter->second).end())
                throw std::runtime_error("No such type in the nestedadapters");

            return *it->second;
        }
    }
    throw std::runtime_error("No such column in the nestedadapters");
}

void CSMWorld::RefIdCollection::copyTo(int index, RefIdCollection& target) const
{
    mData.copyTo(index, target.mData);
}
