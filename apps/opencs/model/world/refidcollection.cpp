#include "refidcollection.hpp"

#include <stdexcept>
#include <memory>

#include <components/esm/esmreader.hpp>

#include "refidadapter.hpp"
#include "refidadapterimp.hpp"
#include "columns.hpp"
#include "nestedtablewrapper.hpp"
#include "nestedcoladapterimp.hpp"

CSMWorld::RefIdColumn::RefIdColumn (int columnId, Display displayType, int flag,
    bool editable, bool userEditable)
    : NestableColumn (columnId, displayType, flag), mEditable (editable), mUserEditable (userEditable)
{}

bool CSMWorld::RefIdColumn::isEditable() const
{
    return mEditable;
}

bool CSMWorld::RefIdColumn::isUserEditable() const
{
    return mUserEditable;
}

const CSMWorld::RefIdAdapter& CSMWorld::RefIdCollection::findAdapter (UniversalId::Type type) const
{
    std::map<UniversalId::Type, RefIdAdapter *>::const_iterator iter = mAdapters.find (type);

    if (iter==mAdapters.end())
        throw std::logic_error ("unsupported type in RefIdCollection");

    return *iter->second;
}

CSMWorld::RefIdCollection::RefIdCollection()
{
    BaseColumns baseColumns;

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Id, ColumnBase::Display_Id,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, false, false));
    baseColumns.mId = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Modification, ColumnBase::Display_RecordState,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, true, false));
    baseColumns.mModified = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_RecordType, ColumnBase::Display_RefRecordType,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, false, false));
    baseColumns.mType = &mColumns.back();

    ModelColumns modelColumns (baseColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Model, ColumnBase::Display_Mesh));
    modelColumns.mModel = &mColumns.back();

    NameColumns nameColumns (modelColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Name, ColumnBase::Display_String));
    nameColumns.mName = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Script, ColumnBase::Display_Script));
    nameColumns.mScript = &mColumns.back();

    InventoryColumns inventoryColumns (nameColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Icon, ColumnBase::Display_Icon));
    inventoryColumns.mIcon = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Weight, ColumnBase::Display_Float));
    inventoryColumns.mWeight = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_CoinValue, ColumnBase::Display_Integer));
    inventoryColumns.mValue = &mColumns.back();

    IngredientColumns ingredientColumns (inventoryColumns);
    mColumns.push_back (RefIdColumn (Columns::ColumnId_EffectList,
        ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    ingredientColumns.mEffects = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> ingredientEffectsMap;
    ingredientEffectsMap.insert(std::make_pair(UniversalId::Type_Ingredient,
        new IngredEffectRefIdAdapter ()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), ingredientEffectsMap));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_EffectId, ColumnBase::Display_IngredEffectId));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_Skill, ColumnBase::Display_EffectSkill));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_Attribute, ColumnBase::Display_EffectAttribute));

    // nested table
    PotionColumns potionColumns (inventoryColumns);
    mColumns.push_back (RefIdColumn (Columns::ColumnId_EffectList,
        ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    potionColumns.mEffects = &mColumns.back(); // see refidadapterimp.hpp
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> effectsMap;
    effectsMap.insert(std::make_pair(UniversalId::Type_Potion,
        new EffectsRefIdAdapter<ESM::Potion> (UniversalId::Type_Potion)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), effectsMap));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_EffectId, ColumnBase::Display_EffectId));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_Skill, ColumnBase::Display_EffectSkill));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_Attribute, ColumnBase::Display_EffectAttribute));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_EffectRange, ColumnBase::Display_EffectRange));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_EffectArea, ColumnBase::Display_String));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_Duration, ColumnBase::Display_Integer)); // reuse from light
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_MinMagnitude, ColumnBase::Display_Integer));
    mColumns.back().addColumn(
        new NestedChildColumn (Columns::ColumnId_MaxMagnitude, ColumnBase::Display_Integer));

    EnchantableColumns enchantableColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Enchantment, ColumnBase::Display_Enchantment));
    enchantableColumns.mEnchantment = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_EnchantmentPoints, ColumnBase::Display_Integer));
    enchantableColumns.mEnchantmentPoints = &mColumns.back();

    ToolColumns toolsColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Quality, ColumnBase::Display_Float));
    toolsColumns.mQuality = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Charges, ColumnBase::Display_Integer));
    toolsColumns.mUses = &mColumns.back();

    ActorColumns actorsColumns (nameColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiHello, ColumnBase::Display_UnsignedInteger16));
    actorsColumns.mHello = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiFlee, ColumnBase::Display_UnsignedInteger8));
    actorsColumns.mFlee = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiFight, ColumnBase::Display_UnsignedInteger8));
    actorsColumns.mFight = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiAlarm, ColumnBase::Display_UnsignedInteger8));
    actorsColumns.mAlarm = &mColumns.back();

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_ActorInventory,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    actorsColumns.mInventory = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> inventoryMap;
    inventoryMap.insert(std::make_pair(UniversalId::Type_Npc,
            new NestedInventoryRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc)));
    inventoryMap.insert(std::make_pair(UniversalId::Type_Creature,
            new NestedInventoryRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), inventoryMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_InventoryItemId, CSMWorld::ColumnBase::Display_Referenceable));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_ItemCount, CSMWorld::ColumnBase::Display_Integer));

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_SpellList,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    actorsColumns.mSpells = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> spellsMap;
    spellsMap.insert(std::make_pair(UniversalId::Type_Npc,
            new NestedSpellRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc)));
    spellsMap.insert(std::make_pair(UniversalId::Type_Creature,
            new NestedSpellRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), spellsMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_SpellId, CSMWorld::ColumnBase::Display_Spell));

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_NpcDestinations,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    actorsColumns.mDestinations = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> destMap;
    destMap.insert(std::make_pair(UniversalId::Type_Npc,
            new NestedTravelRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc)));
    destMap.insert(std::make_pair(UniversalId::Type_Creature,
            new NestedTravelRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), destMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_DestinationCell, CSMWorld::ColumnBase::Display_Cell));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_PosX, CSMWorld::ColumnBase::Display_Float));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_PosY, CSMWorld::ColumnBase::Display_Float));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_PosZ, CSMWorld::ColumnBase::Display_Float));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_RotX, CSMWorld::ColumnBase::Display_Double));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_RotY, CSMWorld::ColumnBase::Display_Double));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_RotZ, CSMWorld::ColumnBase::Display_Double));

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_AiPackageList,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    actorsColumns.mAiPackages = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> aiMap;
    aiMap.insert(std::make_pair(UniversalId::Type_Npc,
            new ActorAiRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc)));
    aiMap.insert(std::make_pair(UniversalId::Type_Creature,
            new ActorAiRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), aiMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiPackageType, CSMWorld::ColumnBase::Display_AiPackageType));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiWanderDist, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiDuration, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiWanderToD, CSMWorld::ColumnBase::Display_Integer));

    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle1, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle2, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle3, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle4, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle5, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle6, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle7, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Idle8, CSMWorld::ColumnBase::Display_Integer));

    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiWanderRepeat, CSMWorld::ColumnBase::Display_Boolean));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiActivateName, CSMWorld::ColumnBase::Display_String));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiTargetId, CSMWorld::ColumnBase::Display_String));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AiTargetCell, CSMWorld::ColumnBase::Display_String));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_PosX, CSMWorld::ColumnBase::Display_Float));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_PosY, CSMWorld::ColumnBase::Display_Float));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_PosZ, CSMWorld::ColumnBase::Display_Float));

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sServiceTable[] =
    {
        { Columns::ColumnId_BuysWeapons, ESM::NPC::Weapon},
        { Columns::ColumnId_BuysArmor, ESM::NPC::Armor},
        { Columns::ColumnId_BuysClothing, ESM::NPC::Clothing},
        { Columns::ColumnId_BuysBooks, ESM::NPC::Books},
        { Columns::ColumnId_BuysIngredients, ESM::NPC::Ingredients},
        { Columns::ColumnId_BuysLockpicks, ESM::NPC::Picks},
        { Columns::ColumnId_BuysProbes, ESM::NPC::Probes},
        { Columns::ColumnId_BuysLights, ESM::NPC::Lights},
        { Columns::ColumnId_BuysApparati, ESM::NPC::Apparatus},
        { Columns::ColumnId_BuysRepairItems, ESM::NPC::RepairItem},
        { Columns::ColumnId_BuysMiscItems, ESM::NPC::Misc},
        { Columns::ColumnId_BuysPotions, ESM::NPC::Potions},
        { Columns::ColumnId_BuysMagicItems, ESM::NPC::MagicItems},
        { Columns::ColumnId_SellsSpells, ESM::NPC::Spells},
        { Columns::ColumnId_Trainer, ESM::NPC::Training},
        { Columns::ColumnId_Spellmaking, ESM::NPC::Spellmaking},
        { Columns::ColumnId_EnchantingService, ESM::NPC::Enchanting},
        { Columns::ColumnId_RepairService, ESM::NPC::Repair},
        { -1, 0 }
    };

    for (int i=0; sServiceTable[i].mName!=-1; ++i)
    {
        mColumns.push_back (RefIdColumn (sServiceTable[i].mName, ColumnBase::Display_Boolean));
        actorsColumns.mServices.insert (std::make_pair (&mColumns.back(), sServiceTable[i].mFlag));
    }

    mColumns.push_back (RefIdColumn (Columns::ColumnId_AutoCalc, ColumnBase::Display_Boolean,
            ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_Refresh));
    const RefIdColumn *autoCalc = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_ApparatusType,
        ColumnBase::Display_ApparatusType));
    const RefIdColumn *apparatusType = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_ArmorType, ColumnBase::Display_ArmorType));
    const RefIdColumn *armorType = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Health, ColumnBase::Display_Integer));
    const RefIdColumn *health = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_ArmorValue, ColumnBase::Display_Integer));
    const RefIdColumn *armor = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_BookType, ColumnBase::Display_BookType));
    const RefIdColumn *bookType = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Skill, ColumnBase::Display_SkillId));
    const RefIdColumn *skill = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Text, ColumnBase::Display_LongString));
    const RefIdColumn *text = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_ClothingType, ColumnBase::Display_ClothingType));
    const RefIdColumn *clothingType = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_WeightCapacity, ColumnBase::Display_Float));
    const RefIdColumn *weightCapacity = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_OrganicContainer, ColumnBase::Display_Boolean));
    const RefIdColumn *organic = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Respawn, ColumnBase::Display_Boolean));
    const RefIdColumn *respawn = &mColumns.back();

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_ContainerContent,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    const RefIdColumn *content = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> contMap;
    contMap.insert(std::make_pair(UniversalId::Type_Container,
            new NestedInventoryRefIdAdapter<ESM::Container> (UniversalId::Type_Container)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), contMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_InventoryItemId, CSMWorld::ColumnBase::Display_Referenceable));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_ItemCount, CSMWorld::ColumnBase::Display_Integer));

    CreatureColumns creatureColumns (actorsColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_CreatureType, ColumnBase::Display_CreatureType));
    creatureColumns.mType = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Scale, ColumnBase::Display_Float));
    creatureColumns.mScale = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_ParentCreature, ColumnBase::Display_Creature));
    creatureColumns.mOriginal = &mColumns.back();

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sCreatureFlagTable[] =
    {
        { Columns::ColumnId_Biped, ESM::Creature::Bipedal },
        { Columns::ColumnId_HasWeapon, ESM::Creature::Weapon },
        { Columns::ColumnId_Swims, ESM::Creature::Swims },
        { Columns::ColumnId_Flies, ESM::Creature::Flies },
        { Columns::ColumnId_Walks, ESM::Creature::Walks },
        { Columns::ColumnId_Essential, ESM::Creature::Essential },
        { -1, 0 }
    };

    // for re-use in NPC records
    const RefIdColumn *essential = 0;

    for (int i=0; sCreatureFlagTable[i].mName!=-1; ++i)
    {
        mColumns.push_back (RefIdColumn (sCreatureFlagTable[i].mName, ColumnBase::Display_Boolean));
        creatureColumns.mFlags.insert (std::make_pair (&mColumns.back(), sCreatureFlagTable[i].mFlag));

        switch (sCreatureFlagTable[i].mFlag)
        {
            case ESM::Creature::Essential: essential = &mColumns.back(); break;
        }
    }

    mColumns.push_back(RefIdColumn(Columns::ColumnId_BloodType, ColumnBase::Display_BloodType));
    // For re-use in NPC records.
    const RefIdColumn *bloodType = &mColumns.back();
    creatureColumns.mBloodType = bloodType;

    creatureColumns.mFlags.insert (std::make_pair (respawn, ESM::Creature::Respawn));

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_CreatureAttributes,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    creatureColumns.mAttributes = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> creaAttrMap;
    creaAttrMap.insert(std::make_pair(UniversalId::Type_Creature, new CreatureAttributesRefIdAdapter()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), creaAttrMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Attribute, CSMWorld::ColumnBase::Display_Attribute, false, false));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_AttributeValue, CSMWorld::ColumnBase::Display_Integer));

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_CreatureAttack,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    creatureColumns.mAttacks = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> attackMap;
    attackMap.insert(std::make_pair(UniversalId::Type_Creature, new CreatureAttackRefIdAdapter()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), attackMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_CreatureAttack, CSMWorld::ColumnBase::Display_Integer, false, false));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_MinAttack, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_MaxAttack, CSMWorld::ColumnBase::Display_Integer));

    // Nested list
    mColumns.push_back(RefIdColumn (Columns::ColumnId_CreatureMisc,
        ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_List));
    creatureColumns.mMisc = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> creaMiscMap;
    creaMiscMap.insert(std::make_pair(UniversalId::Type_Creature, new CreatureMiscRefIdAdapter()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), creaMiscMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Level, CSMWorld::ColumnBase::Display_Integer,
            ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_Refresh));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Health, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Mana, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Fatigue, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_SoulPoints, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_CombatState, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_MagicState, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_StealthState, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Gold, CSMWorld::ColumnBase::Display_Integer));

    mColumns.push_back (RefIdColumn (Columns::ColumnId_OpenSound, ColumnBase::Display_Sound));
    const RefIdColumn *openSound = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_CloseSound, ColumnBase::Display_Sound));
    const RefIdColumn *closeSound = &mColumns.back();

    LightColumns lightColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Duration, ColumnBase::Display_Integer));
    lightColumns.mTime = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Radius, ColumnBase::Display_Integer));
    lightColumns.mRadius = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Colour, ColumnBase::Display_Colour));
    lightColumns.mColor = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Sound, ColumnBase::Display_Sound));
    lightColumns.mSound = &mColumns.back();

    mColumns.push_back(RefIdColumn(Columns::ColumnId_EmitterType, ColumnBase::Display_EmitterType));
    lightColumns.mEmitterType = &mColumns.back();

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sLightFlagTable[] =
    {
        { Columns::ColumnId_Dynamic, ESM::Light::Dynamic },
        { Columns::ColumnId_Portable, ESM::Light::Carry },
        { Columns::ColumnId_NegativeLight, ESM::Light::Negative },
        { Columns::ColumnId_Fire, ESM::Light::Fire },
        { Columns::ColumnId_OffByDefault, ESM::Light::OffDefault },
        { -1, 0 }
    };

    for (int i=0; sLightFlagTable[i].mName!=-1; ++i)
    {
        mColumns.push_back (RefIdColumn (sLightFlagTable[i].mName, ColumnBase::Display_Boolean));
        lightColumns.mFlags.insert (std::make_pair (&mColumns.back(), sLightFlagTable[i].mFlag));
    }

    mColumns.push_back (RefIdColumn (Columns::ColumnId_IsKey, ColumnBase::Display_Boolean));
    const RefIdColumn *key = &mColumns.back();

    NpcColumns npcColumns (actorsColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Race, ColumnBase::Display_Race));
    npcColumns.mRace = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Class, ColumnBase::Display_Class));
    npcColumns.mClass = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Faction, ColumnBase::Display_Faction));
    npcColumns.mFaction = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::Columnid_Hair, ColumnBase::Display_BodyPart));
    npcColumns.mHair = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Head, ColumnBase::Display_BodyPart));
    npcColumns.mHead = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Gender, ColumnBase::Display_GenderNpc));
    npcColumns.mGender = &mColumns.back();

    npcColumns.mFlags.insert (std::make_pair (essential, ESM::NPC::Essential));

    npcColumns.mFlags.insert (std::make_pair (respawn, ESM::NPC::Respawn));

    npcColumns.mFlags.insert (std::make_pair (autoCalc, ESM::NPC::Autocalc));

    // Re-used from Creature records.
    npcColumns.mBloodType = bloodType;

    // Need a way to add a table of stats and values (rather than adding a long list of
    // entries in the dialogue subview) E.g. attributes+stats(health, mana, fatigue), skills
    // These needs to be driven from the autocalculated setting.

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_NpcAttributes,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    npcColumns.mAttributes = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> attrMap;
    attrMap.insert(std::make_pair(UniversalId::Type_Npc, new NpcAttributesRefIdAdapter()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), attrMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Attribute, CSMWorld::ColumnBase::Display_Attribute, false, false));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_UChar, CSMWorld::ColumnBase::Display_UnsignedInteger8));

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_NpcSkills,
            ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    npcColumns.mSkills = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> skillsMap;
    skillsMap.insert(std::make_pair(UniversalId::Type_Npc, new NpcSkillsRefIdAdapter()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), skillsMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Skill, CSMWorld::ColumnBase::Display_SkillId, false, false));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_UChar, CSMWorld::ColumnBase::Display_UnsignedInteger8));

    // Nested list
    mColumns.push_back(RefIdColumn (Columns::ColumnId_NpcMisc,
        ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_List));
    npcColumns.mMisc = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> miscMap;
    miscMap.insert(std::make_pair(UniversalId::Type_Npc, new NpcMiscRefIdAdapter()));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), miscMap));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Level, CSMWorld::ColumnBase::Display_SignedInteger16));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Health, CSMWorld::ColumnBase::Display_UnsignedInteger16));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Mana, CSMWorld::ColumnBase::Display_UnsignedInteger16));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Fatigue, CSMWorld::ColumnBase::Display_UnsignedInteger16));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_NpcDisposition, CSMWorld::ColumnBase::Display_UnsignedInteger8));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_NpcReputation, CSMWorld::ColumnBase::Display_UnsignedInteger8));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_NpcRank, CSMWorld::ColumnBase::Display_UnsignedInteger8));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_Gold, CSMWorld::ColumnBase::Display_Integer));
    mColumns.back().addColumn(
            new RefIdColumn (Columns::ColumnId_NpcPersistence, CSMWorld::ColumnBase::Display_Boolean));

    WeaponColumns weaponColumns (enchantableColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_WeaponType, ColumnBase::Display_WeaponType));
    weaponColumns.mType = &mColumns.back();

    weaponColumns.mHealth = health;

    mColumns.push_back (RefIdColumn (Columns::ColumnId_WeaponSpeed, ColumnBase::Display_Float));
    weaponColumns.mSpeed = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_WeaponReach, ColumnBase::Display_Float));
    weaponColumns.mReach = &mColumns.back();

    for (int i=0; i<3; ++i)
    {
        const RefIdColumn **column =
            i==0 ? weaponColumns.mChop : (i==1 ? weaponColumns.mSlash : weaponColumns.mThrust);

        for (int j=0; j<2; ++j)
        {
            mColumns.push_back (
                RefIdColumn (Columns::ColumnId_MinChop+i*2+j, ColumnBase::Display_Integer));
            column[j] = &mColumns.back();
        }
    }

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sWeaponFlagTable[] =
    {
        { Columns::ColumnId_Magical, ESM::Weapon::Magical },
        { Columns::ColumnId_Silver, ESM::Weapon::Silver },
        { -1, 0 }
    };

    for (int i=0; sWeaponFlagTable[i].mName!=-1; ++i)
    {
        mColumns.push_back (RefIdColumn (sWeaponFlagTable[i].mName, ColumnBase::Display_Boolean));
        weaponColumns.mFlags.insert (std::make_pair (&mColumns.back(), sWeaponFlagTable[i].mFlag));
    }

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_PartRefList, ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    const RefIdColumn *partRef = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> partMap;
    partMap.insert(std::make_pair(UniversalId::Type_Armor,
        new BodyPartRefIdAdapter<ESM::Armor> (UniversalId::Type_Armor)));
    partMap.insert(std::make_pair(UniversalId::Type_Clothing,
        new BodyPartRefIdAdapter<ESM::Clothing> (UniversalId::Type_Clothing)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), partMap));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_PartRefType, CSMWorld::ColumnBase::Display_PartRefType));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_PartRefMale, CSMWorld::ColumnBase::Display_BodyPart));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_PartRefFemale, CSMWorld::ColumnBase::Display_BodyPart));

    LevListColumns levListColumns (baseColumns);

    // Nested table
    mColumns.push_back(RefIdColumn (Columns::ColumnId_LevelledList,
        ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue));
    levListColumns.mLevList = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> levListMap;
    levListMap.insert(std::make_pair(UniversalId::Type_CreatureLevelledList,
        new NestedLevListRefIdAdapter<ESM::CreatureLevList> (UniversalId::Type_CreatureLevelledList)));
    levListMap.insert(std::make_pair(UniversalId::Type_ItemLevelledList,
        new NestedLevListRefIdAdapter<ESM::ItemLevList> (UniversalId::Type_ItemLevelledList)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), levListMap));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_LevelledItemId, CSMWorld::ColumnBase::Display_Referenceable));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_LevelledItemLevel, CSMWorld::ColumnBase::Display_Integer));

    // Nested list
    mColumns.push_back(RefIdColumn (Columns::ColumnId_LevelledList,
        ColumnBase::Display_NestedHeader, ColumnBase::Flag_Dialogue | ColumnBase::Flag_Dialogue_List));
    levListColumns.mNestedListLevList = &mColumns.back();
    std::map<UniversalId::Type, NestedRefIdAdapterBase*> nestedListLevListMap;
    nestedListLevListMap.insert(std::make_pair(UniversalId::Type_CreatureLevelledList,
        new NestedListLevListRefIdAdapter<ESM::CreatureLevList> (UniversalId::Type_CreatureLevelledList)));
    nestedListLevListMap.insert(std::make_pair(UniversalId::Type_ItemLevelledList,
        new NestedListLevListRefIdAdapter<ESM::ItemLevList> (UniversalId::Type_ItemLevelledList)));
    mNestedAdapters.push_back (std::make_pair(&mColumns.back(), nestedListLevListMap));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_LevelledItemTypeEach, CSMWorld::ColumnBase::Display_Boolean));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_LevelledItemType, CSMWorld::ColumnBase::Display_Boolean));
    mColumns.back().addColumn(
        new RefIdColumn (Columns::ColumnId_LevelledItemChanceNone, CSMWorld::ColumnBase::Display_UnsignedInteger8));

    mAdapters.insert (std::make_pair (UniversalId::Type_Activator,
        new NameRefIdAdapter<ESM::Activator> (UniversalId::Type_Activator, nameColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Potion,
        new PotionRefIdAdapter (potionColumns, autoCalc)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Apparatus,
        new ApparatusRefIdAdapter (inventoryColumns, apparatusType, toolsColumns.mQuality)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Armor,
        new ArmorRefIdAdapter (enchantableColumns, armorType, health, armor, partRef)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Book,
        new BookRefIdAdapter (enchantableColumns, bookType, skill, text)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Clothing,
        new ClothingRefIdAdapter (enchantableColumns, clothingType, partRef)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Container,
        new ContainerRefIdAdapter (nameColumns, weightCapacity, organic, respawn, content)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Creature,
        new CreatureRefIdAdapter (creatureColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Door,
        new DoorRefIdAdapter (nameColumns, openSound, closeSound)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Ingredient,
        new IngredientRefIdAdapter (ingredientColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_CreatureLevelledList,
        new LevelledListRefIdAdapter<ESM::CreatureLevList> (
        UniversalId::Type_CreatureLevelledList, levListColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_ItemLevelledList,
        new LevelledListRefIdAdapter<ESM::ItemLevList> (UniversalId::Type_ItemLevelledList, levListColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Light,
        new LightRefIdAdapter (lightColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Lockpick,
        new ToolRefIdAdapter<ESM::Lockpick> (UniversalId::Type_Lockpick, toolsColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Miscellaneous,
        new MiscRefIdAdapter (inventoryColumns, key)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Npc,
        new NpcRefIdAdapter (npcColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Probe,
        new ToolRefIdAdapter<ESM::Probe> (UniversalId::Type_Probe, toolsColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Repair,
        new ToolRefIdAdapter<ESM::Repair> (UniversalId::Type_Repair, toolsColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Static,
        new ModelRefIdAdapter<ESM::Static> (UniversalId::Type_Static, modelColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Weapon,
        new WeaponRefIdAdapter (weaponColumns)));
}

CSMWorld::RefIdCollection::~RefIdCollection()
{
    for (std::map<UniversalId::Type, RefIdAdapter *>::iterator iter (mAdapters.begin());
         iter!=mAdapters.end(); ++iter)
         delete iter->second;

    for (std::vector<std::pair<const ColumnBase*, std::map<UniversalId::Type, NestedRefIdAdapterBase*> > >::iterator iter (mNestedAdapters.begin());
         iter!=mNestedAdapters.end(); ++iter)
    {
        for (std::map<UniversalId::Type, NestedRefIdAdapterBase *>::iterator it ((iter->second).begin());
            it!=(iter->second).end(); ++it)
            delete it->second;
    }
}

int CSMWorld::RefIdCollection::getSize() const
{
    return mData.getSize();
}

std::string CSMWorld::RefIdCollection::getId (int index) const
{
    return getData (index, 0).toString().toUtf8().constData();
}

int CSMWorld::RefIdCollection::getIndex (const std::string& id) const
{
    int index = searchId (id);

    if (index==-1)
        throw std::runtime_error ("invalid ID: " + id);

    return index;
}

int CSMWorld::RefIdCollection::getColumns() const
{
    return mColumns.size();
}

const CSMWorld::ColumnBase& CSMWorld::RefIdCollection::getColumn (int column) const
{
    return mColumns.at (column);
}

QVariant CSMWorld::RefIdCollection::getData (int index, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (index);

    const RefIdAdapter& adaptor = findAdapter (localIndex.second);

    return adaptor.getData (&mColumns.at (column), mData, localIndex.first);
}

QVariant CSMWorld::RefIdCollection::getNestedData (int row, int column, int subRow, int subColumn) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex(row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.getNestedData(&mColumns.at (column), mData, localIndex.first, subRow, subColumn);
}

void CSMWorld::RefIdCollection::setData (int index, int column, const QVariant& data)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (index);

    const RefIdAdapter& adaptor = findAdapter (localIndex.second);

    adaptor.setData (&mColumns.at (column), mData, localIndex.first, data);
}

void CSMWorld::RefIdCollection::setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    nestedAdapter.setNestedData(&mColumns.at (column), mData, localIndex.first, data, subRow, subColumn);
    return;
}

void CSMWorld::RefIdCollection::removeRows (int index, int count)
{
    mData.erase (index, count);
}

void CSMWorld::RefIdCollection::removeNestedRows(int row, int column, int subRow)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    nestedAdapter.removeNestedRow(&mColumns.at (column), mData, localIndex.first, subRow);
    return;
}

void CSMWorld::RefIdCollection::appendBlankRecord (const std::string& id, UniversalId::Type type)
{
    mData.appendRecord (type, id, false);
}

int CSMWorld::RefIdCollection::searchId (const std::string& id) const
{
    RefIdData::LocalIndex localIndex = mData.searchId (id);

    if (localIndex.first==-1)
        return -1;

    return mData.localToGlobalIndex (localIndex);
}

void CSMWorld::RefIdCollection::replace (int index, const RecordBase& record)
{
    mData.getRecord (mData.globalToLocalIndex (index)).assign (record);
}

void CSMWorld::RefIdCollection::cloneRecord(const std::string& origin,
                                     const std::string& destination,
                                     const CSMWorld::UniversalId::Type type)
{
        std::unique_ptr<RecordBase> newRecord(mData.getRecord(mData.searchId(origin)).modifiedCopy());
        mAdapters.find(type)->second->setId(*newRecord, destination);
        mData.insertRecord(*newRecord, type, destination);
}

bool CSMWorld::RefIdCollection::touchRecord(const std::string& id)
{
    throw std::runtime_error("RefIdCollection::touchRecord is unimplemented");
    return false;
}

void CSMWorld::RefIdCollection::appendRecord (const RecordBase& record,
    UniversalId::Type type)
{
    std::string id = findAdapter (type).getId (record);

    int index = mData.getAppendIndex (type);

    mData.appendRecord (type, id, false);

    mData.getRecord (mData.globalToLocalIndex (index)).assign (record);
}

const CSMWorld::RecordBase& CSMWorld::RefIdCollection::getRecord (const std::string& id) const
{
    return mData.getRecord (mData.searchId (id));
}

const CSMWorld::RecordBase& CSMWorld::RefIdCollection::getRecord (int index) const
{
    return mData.getRecord (mData.globalToLocalIndex (index));
}

void CSMWorld::RefIdCollection::load (ESM::ESMReader& reader, bool base, UniversalId::Type type)
{
    mData.load(reader, base, type);
}

int CSMWorld::RefIdCollection::getAppendIndex (const std::string& id, UniversalId::Type type) const
{
    return mData.getAppendIndex (type);
}

std::vector<std::string> CSMWorld::RefIdCollection::getIds (bool listDeleted) const
{
    return mData.getIds (listDeleted);
}

bool CSMWorld::RefIdCollection::reorderRows (int baseIndex, const std::vector<int>& newOrder)
{
    return false;
}

void CSMWorld::RefIdCollection::save (int index, ESM::ESMWriter& writer) const
{
    mData.save (index, writer);
}

const CSMWorld::RefIdData& CSMWorld::RefIdCollection::getDataSet() const
{
    return mData;
}

int CSMWorld::RefIdCollection::getNestedRowsCount(int row, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.getNestedRowsCount(&mColumns.at(column), mData, localIndex.first);
}

int CSMWorld::RefIdCollection::getNestedColumnsCount(int row, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.getNestedColumnsCount(&mColumns.at(column), mData);
}

CSMWorld::NestableColumn *CSMWorld::RefIdCollection::getNestableColumn(int column)
{
    return &mColumns.at(column);
}

void CSMWorld::RefIdCollection::addNestedRow(int row, int col, int position)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(col), localIndex.second);

    nestedAdapter.addNestedRow(&mColumns.at(col), mData, localIndex.first, position);
    return;
}

void CSMWorld::RefIdCollection::setNestedTable(int row, int column, const CSMWorld::NestedTableWrapperBase& nestedTable)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    nestedAdapter.setNestedTable(&mColumns.at(column), mData, localIndex.first, nestedTable);
    return;
}

CSMWorld::NestedTableWrapperBase* CSMWorld::RefIdCollection::nestedTable(int row, int column) const
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (row);
    const CSMWorld::NestedRefIdAdapterBase& nestedAdapter = getNestedAdapter(mColumns.at(column), localIndex.second);

    return nestedAdapter.nestedTable(&mColumns.at(column), mData, localIndex.first);
}

const CSMWorld::NestedRefIdAdapterBase& CSMWorld::RefIdCollection::getNestedAdapter(const CSMWorld::ColumnBase &column, UniversalId::Type type) const
{
    for (std::vector<std::pair<const ColumnBase*, std::map<UniversalId::Type, NestedRefIdAdapterBase*> > >::const_iterator iter (mNestedAdapters.begin());
         iter!=mNestedAdapters.end(); ++iter)
    {
        if ((iter->first) == &column)
        {
            std::map<UniversalId::Type, NestedRefIdAdapterBase*>::const_iterator it =
                (iter->second).find(type);

            if (it == (iter->second).end())
                throw std::runtime_error("No such type in the nestedadapters");

            return *it->second;
        }
    }
    throw std::runtime_error("No such column in the nestedadapters");
}

void CSMWorld::RefIdCollection::copyTo (int index, RefIdCollection& target) const
{
    mData.copyTo (index, target.mData);
}
