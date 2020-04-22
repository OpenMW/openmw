#include "referenceablecheck.hpp"

#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "../prefs/state.hpp"

#include "../world/record.hpp"
#include "../world/universalid.hpp"

CSMTools::ReferenceableCheckStage::ReferenceableCheckStage(
    const CSMWorld::RefIdData& referenceable, const CSMWorld::IdCollection<ESM::Race >& races,
    const CSMWorld::IdCollection<ESM::Class>& classes,
    const CSMWorld::IdCollection<ESM::Faction>& faction,
    const CSMWorld::IdCollection<ESM::Script>& scripts,
    const CSMWorld::Resources& models,
    const CSMWorld::Resources& icons,
    const CSMWorld::IdCollection<ESM::BodyPart>& bodyparts)
   :mReferencables(referenceable),
    mRaces(races),
    mClasses(classes),
    mFactions(faction),
    mScripts(scripts),
    mModels(models),
    mIcons(icons),
    mBodyParts(bodyparts),
    mPlayerPresent(false)
{
    mIgnoreBaseRecords = false;
}

void CSMTools::ReferenceableCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    //Checks for books, than, when stage is above mBooksSize goes to other checks, with (stage - PrevSum) as stage.
    const int bookSize(mReferencables.getBooks().getSize());

    if (stage < bookSize)
    {
        bookCheck(stage, mReferencables.getBooks(), messages);
        return;
    }

    stage -= bookSize;

    const int activatorSize(mReferencables.getActivators().getSize());

    if (stage < activatorSize)
    {
        activatorCheck(stage, mReferencables.getActivators(), messages);
        return;
    }

    stage -= activatorSize;

    const int potionSize(mReferencables.getPotions().getSize());

    if (stage < potionSize)
    {
        potionCheck(stage, mReferencables.getPotions(), messages);
        return;
    }

    stage -= potionSize;

    const int apparatusSize(mReferencables.getApparati().getSize());

    if (stage < apparatusSize)
    {
        apparatusCheck(stage, mReferencables.getApparati(), messages);
        return;
    }

    stage -= apparatusSize;

    const int armorSize(mReferencables.getArmors().getSize());

    if (stage < armorSize)
    {
        armorCheck(stage, mReferencables.getArmors(), messages);
        return;
    }

    stage -= armorSize;

    const int clothingSize(mReferencables.getClothing().getSize());

    if (stage < clothingSize)
    {
        clothingCheck(stage, mReferencables.getClothing(), messages);
        return;
    }

    stage -= clothingSize;

    const int containerSize(mReferencables.getContainers().getSize());

    if (stage < containerSize)
    {
        containerCheck(stage, mReferencables.getContainers(), messages);
        return;
    }

    stage -= containerSize;

    const int doorSize(mReferencables.getDoors().getSize());

    if (stage < doorSize)
    {
        doorCheck(stage, mReferencables.getDoors(), messages);
        return;
    }

    stage -= doorSize;

    const int ingredientSize(mReferencables.getIngredients().getSize());

    if (stage < ingredientSize)
    {
        ingredientCheck(stage, mReferencables.getIngredients(), messages);
        return;
    }

    stage -= ingredientSize;

    const int creatureLevListSize(mReferencables.getCreatureLevelledLists().getSize());

    if (stage < creatureLevListSize)
    {
        creaturesLevListCheck(stage, mReferencables.getCreatureLevelledLists(), messages);
        return;
    }

    stage -= creatureLevListSize;

    const int itemLevelledListSize(mReferencables.getItemLevelledList().getSize());

    if (stage < itemLevelledListSize)
    {
        itemLevelledListCheck(stage, mReferencables.getItemLevelledList(), messages);
        return;
    }

    stage -= itemLevelledListSize;

    const int lightSize(mReferencables.getLights().getSize());

    if (stage < lightSize)
    {
        lightCheck(stage, mReferencables.getLights(), messages);
        return;
    }

    stage -= lightSize;

    const int lockpickSize(mReferencables.getLocpicks().getSize());

    if (stage < lockpickSize)
    {
        lockpickCheck(stage, mReferencables.getLocpicks(), messages);
        return;
    }

    stage -= lockpickSize;

    const int miscSize(mReferencables.getMiscellaneous().getSize());

    if (stage < miscSize)
    {
        miscCheck(stage, mReferencables.getMiscellaneous(), messages);
        return;
    }

    stage -= miscSize;

    const int npcSize(mReferencables.getNPCs().getSize());

    if (stage < npcSize)
    {
        npcCheck(stage, mReferencables.getNPCs(), messages);
        return;
    }

    stage -= npcSize;

    const int weaponSize(mReferencables.getWeapons().getSize());

    if (stage < weaponSize)
    {
        weaponCheck(stage, mReferencables.getWeapons(), messages);
        return;
    }

    stage -= weaponSize;

    const int probeSize(mReferencables.getProbes().getSize());

    if (stage < probeSize)
    {
        probeCheck(stage, mReferencables.getProbes(), messages);
        return;
    }

    stage -= probeSize;

    const int repairSize(mReferencables.getRepairs().getSize());

    if (stage < repairSize)
    {
        repairCheck(stage, mReferencables.getRepairs(), messages);
        return;
    }

    stage -= repairSize;

    const int staticSize(mReferencables.getStatics().getSize());

    if (stage < staticSize)
    {
        staticCheck(stage, mReferencables.getStatics(), messages);
        return;
    }

    stage -= staticSize;

    const int creatureSize(mReferencables.getCreatures().getSize());

    if (stage < creatureSize)
    {
        creatureCheck(stage, mReferencables.getCreatures(), messages);
        return;
    }
// if we come that far, we are about to perform our last, final check.
    finalCheck(messages);
    return;
}

int CSMTools::ReferenceableCheckStage::setup()
{
    mPlayerPresent = false;
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mReferencables.getSize() + 1;
}

void CSMTools::ReferenceableCheckStage::bookCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Book >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Book& book = (dynamic_cast<const CSMWorld::Record<ESM::Book>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Book, book.mId);

    inventoryItemCheck<ESM::Book>(book, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Book>(book, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::activatorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Activator >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Activator& activator = (dynamic_cast<const CSMWorld::Record<ESM::Activator>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Activator, activator.mId);

    if (activator.mModel.empty())
        messages.add(id, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(activator.mModel) == -1)
        messages.add(id, "Model '" + activator.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Activator>(activator, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::potionCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Potion >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Potion& potion = (dynamic_cast<const CSMWorld::Record<ESM::Potion>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Potion, potion.mId);

    inventoryItemCheck<ESM::Potion>(potion, messages, id.toString());
    /// \todo Check magic effects for validity

    // Check that mentioned scripts exist
    scriptCheck<ESM::Potion>(potion, messages, id.toString());
}


void CSMTools::ReferenceableCheckStage::apparatusCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Apparatus >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Apparatus& apparatus = (dynamic_cast<const CSMWorld::Record<ESM::Apparatus>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Apparatus, apparatus.mId);

    inventoryItemCheck<ESM::Apparatus>(apparatus, messages, id.toString());

    toolCheck<ESM::Apparatus>(apparatus, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Apparatus>(apparatus, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::armorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Armor >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Armor& armor = (dynamic_cast<const CSMWorld::Record<ESM::Armor>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Armor, armor.mId);

    inventoryItemCheck<ESM::Armor>(armor, messages, id.toString(), true);

    // Armor should have positive armor class, but 0 class is not an error
    if (armor.mData.mArmor < 0)
        messages.add(id, "Armor class is negative", "", CSMDoc::Message::Severity_Error);

    // Armor durability must be a positive number
    if (armor.mData.mHealth <= 0)
        messages.add(id, "Durability is non-positive", "", CSMDoc::Message::Severity_Error);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Armor>(armor, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::clothingCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Clothing >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Clothing& clothing = (dynamic_cast<const CSMWorld::Record<ESM::Clothing>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Clothing, clothing.mId);
    inventoryItemCheck<ESM::Clothing>(clothing, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Clothing>(clothing, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::containerCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Container >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Container& container = (dynamic_cast<const CSMWorld::Record<ESM::Container>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Container, container.mId);

    //checking for name
    if (container.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    //Checking for model
    if (container.mModel.empty())
        messages.add(id, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(container.mModel) == -1)
        messages.add(id, "Model '" + container.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    //Checking for capacity (weight)
    if (container.mWeight < 0) //0 is allowed
        messages.add(id, "Capacity is negative", "", CSMDoc::Message::Severity_Error);
    
    //checking contained items
    inventoryListCheck(container.mInventory.mList, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Container>(container, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::creatureCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::Creature >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Creature& creature = (dynamic_cast<const CSMWorld::Record<ESM::Creature>&>(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Creature, creature.mId);

    if (creature.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    if (creature.mModel.empty())
        messages.add(id, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(creature.mModel) == -1)
        messages.add(id, "Model '" + creature.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    //stats checks
    if (creature.mData.mLevel <= 0)
        messages.add(id, "Level is non-positive", "", CSMDoc::Message::Severity_Warning);

    if (creature.mData.mStrength < 0)
        messages.add(id, "Strength is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mIntelligence < 0)
        messages.add(id, "Intelligence is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mWillpower < 0)
        messages.add(id, "Willpower is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mAgility < 0)
        messages.add(id, "Agility is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mSpeed < 0)
        messages.add(id, "Speed is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mEndurance < 0)
        messages.add(id, "Endurance is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mPersonality < 0)
        messages.add(id, "Personality is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mLuck < 0)
        messages.add(id, "Luck is negative", "", CSMDoc::Message::Severity_Warning);

    if (creature.mData.mCombat < 0)
        messages.add(id, "Combat is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mMagic < 0)
        messages.add(id, "Magic is negative", "", CSMDoc::Message::Severity_Warning);
    if (creature.mData.mStealth < 0)
        messages.add(id, "Stealth is negative", "", CSMDoc::Message::Severity_Warning);

    if (creature.mData.mHealth < 0)
        messages.add(id, "Health is negative", "", CSMDoc::Message::Severity_Error);
    if (creature.mData.mMana < 0)
        messages.add(id, "Magicka is negative", "", CSMDoc::Message::Severity_Error);
    if (creature.mData.mFatigue < 0)
        messages.add(id, "Fatigue is negative", "", CSMDoc::Message::Severity_Error);

    if (creature.mData.mSoul < 0)
        messages.add(id, "Soul value is negative", "", CSMDoc::Message::Severity_Error);

    if (creature.mAiData.mAlarm > 100)
        messages.add(id, "Alarm rating is over 100", "", CSMDoc::Message::Severity_Warning);
    if (creature.mAiData.mFight > 100)
        messages.add(id, "Fight rating is over 100", "", CSMDoc::Message::Severity_Warning);
    if (creature.mAiData.mFlee > 100)
        messages.add(id, "Flee rating is over 100", "", CSMDoc::Message::Severity_Warning);

    for (int i = 0; i < 6; ++i)
    {
        if (creature.mData.mAttack[i] < 0)
            messages.add(id, "Attack " + std::to_string(i/2 + 1) + " has negative" + (i % 2 == 0 ? " minimum " : " maximum ") + "damage", "", CSMDoc::Message::Severity_Error);
        if (i % 2 == 0 && creature.mData.mAttack[i] > creature.mData.mAttack[i+1])
            messages.add(id, "Attack " + std::to_string(i/2 + 1) + " has minimum damage higher than maximum damage", "", CSMDoc::Message::Severity_Error);
    }

    if (creature.mData.mGold < 0)
        messages.add(id, "Gold count is negative", "", CSMDoc::Message::Severity_Error);

    if (creature.mScale == 0)
        messages.add(id, "Scale is equal to zero", "", CSMDoc::Message::Severity_Error);

    if (!creature.mOriginal.empty())
    {
        CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(creature.mOriginal);
        if (index.first == -1)
            messages.add(id, "Parent creature '" + creature.mOriginal + "' does not exist", "", CSMDoc::Message::Severity_Error);
        else if (index.second != CSMWorld::UniversalId::Type_Creature)
            messages.add(id, "'" + creature.mOriginal + "' is not a creature", "", CSMDoc::Message::Severity_Error);
    }

    // Check inventory
    inventoryListCheck(creature.mInventory.mList, messages, id.toString());
 
    // Check that mentioned scripts exist
    scriptCheck<ESM::Creature>(creature, messages, id.toString());
    /// \todo Check spells, teleport table, AI data and AI packages for validity
}

void CSMTools::ReferenceableCheckStage::doorCheck(
    int stage, const CSMWorld::RefIdDataContainer< ESM::Door >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Door& door = (dynamic_cast<const CSMWorld::Record<ESM::Door>&>(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Door, door.mId);

    //usual, name or model
    if (door.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    if (door.mModel.empty())
        messages.add(id, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(door.mModel) == -1)
        messages.add(id, "Model '" + door.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Door>(door, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::ingredientCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Ingredient >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Ingredient& ingredient = (dynamic_cast<const CSMWorld::Record<ESM::Ingredient>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Ingredient, ingredient.mId);

    inventoryItemCheck<ESM::Ingredient>(ingredient, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Ingredient>(ingredient, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::creaturesLevListCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::CreatureLevList >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::CreatureLevList& CreatureLevList = (dynamic_cast<const CSMWorld::Record<ESM::CreatureLevList>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_CreatureLevelledList, CreatureLevList.mId); //CreatureLevList but Type_CreatureLevelledList :/

    listCheck<ESM::CreatureLevList>(CreatureLevList, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::itemLevelledListCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::ItemLevList >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::ItemLevList& ItemLevList = (dynamic_cast<const CSMWorld::Record<ESM::ItemLevList>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_ItemLevelledList, ItemLevList.mId);

    listCheck<ESM::ItemLevList>(ItemLevList, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::lightCheck(
    int stage, const CSMWorld::RefIdDataContainer< ESM::Light >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Light& light = (dynamic_cast<const CSMWorld::Record<ESM::Light>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Light, light.mId);

    if (light.mData.mRadius < 0)
        messages.add(id, "Light radius is negative", "", CSMDoc::Message::Severity_Error);

    if (light.mData.mFlags & ESM::Light::Carry)
        inventoryItemCheck<ESM::Light>(light, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Light>(light, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::lockpickCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Lockpick >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Lockpick& lockpick = (dynamic_cast<const CSMWorld::Record<ESM::Lockpick>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Lockpick, lockpick.mId);

    inventoryItemCheck<ESM::Lockpick>(lockpick, messages, id.toString());

    toolCheck<ESM::Lockpick>(lockpick, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Lockpick>(lockpick, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::miscCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Miscellaneous >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Miscellaneous& miscellaneous = (dynamic_cast<const CSMWorld::Record<ESM::Miscellaneous>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Miscellaneous, miscellaneous.mId);

    inventoryItemCheck<ESM::Miscellaneous>(miscellaneous, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Miscellaneous>(miscellaneous, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::npcCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::NPC >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    if (baseRecord.isDeleted())
        return;

    const ESM::NPC& npc = (dynamic_cast<const CSMWorld::Record<ESM::NPC>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Npc, npc.mId);

    //Detect if player is present
    if (Misc::StringUtils::ciEqual(npc.mId, "player")) //Happy now, scrawl?
        mPlayerPresent = true;

    // Skip "Base" records (setting!)
    if (mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly)
        return;

    short level(npc.mNpdt.mLevel);
    int gold(npc.mNpdt.mGold);

    if (npc.mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS) //12 = autocalculated
    {
        if ((npc.mFlags & ESM::NPC::Autocalc) == 0) //0x0010 = autocalculated flag
        {
            messages.add(id, "NPC with autocalculated stats doesn't have autocalc flag turned on", "", CSMDoc::Message::Severity_Error); //should not happen?
            return;
        }
    }
    else
    {
        if (npc.mNpdt.mStrength == 0)
            messages.add(id, "Strength is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mIntelligence == 0)
            messages.add(id, "Intelligence is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mWillpower == 0)
            messages.add(id, "Willpower is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mAgility == 0)
            messages.add(id, "Agility is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mSpeed == 0)
            messages.add(id, "Speed is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mEndurance == 0)
            messages.add(id, "Endurance is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mPersonality == 0)
            messages.add(id, "Personality is equal to zero", "", CSMDoc::Message::Severity_Warning);
        if (npc.mNpdt.mLuck == 0)
            messages.add(id, "Luck is equal to zero", "", CSMDoc::Message::Severity_Warning);
    }

    if (level <= 0)
        messages.add(id, "Level is non-positive", "", CSMDoc::Message::Severity_Warning);

    if (npc.mAiData.mAlarm > 100)
        messages.add(id, "Alarm rating is over 100", "", CSMDoc::Message::Severity_Warning);
    if (npc.mAiData.mFight > 100)
        messages.add(id, "Fight rating is over 100", "", CSMDoc::Message::Severity_Warning);
    if (npc.mAiData.mFlee > 100)
        messages.add(id, "Flee rating is over 100", "", CSMDoc::Message::Severity_Warning);

    if (gold < 0)
        messages.add(id, "Gold count is negative", "", CSMDoc::Message::Severity_Error);

    if (npc.mName.empty())
        messages.add(id, "Name is missing", "", CSMDoc::Message::Severity_Error);

    if (npc.mClass.empty())
        messages.add(id, "Class is missing", "", CSMDoc::Message::Severity_Error);
    else if (mClasses.searchId (npc.mClass) == -1)
        messages.add(id, "Class '" + npc.mClass + "' does not exist", "", CSMDoc::Message::Severity_Error);

    if (npc.mRace.empty())
        messages.add(id, "Race is missing", "", CSMDoc::Message::Severity_Error);
    else if (mRaces.searchId (npc.mRace) == -1)
        messages.add(id, "Race '" + npc.mRace + "' does not exist", "", CSMDoc::Message::Severity_Error);

    if (!npc.mFaction.empty() && mFactions.searchId(npc.mFaction) == -1)
        messages.add(id, "Faction '" + npc.mFaction + "' does not exist", "", CSMDoc::Message::Severity_Error);

    if (npc.mHead.empty())
        messages.add(id, "Head is missing", "", CSMDoc::Message::Severity_Error);
    else
    {
        if (mBodyParts.searchId(npc.mHead) == -1)
            messages.add(id, "Head body part '" + npc.mHead + "' does not exist", "", CSMDoc::Message::Severity_Error);
        /// \todo Check gender, race and other body parts stuff validity for the specific NPC
    }

    if (npc.mHair.empty())
        messages.add(id, "Hair is missing", "", CSMDoc::Message::Severity_Error);
    else
    {
        if (mBodyParts.searchId(npc.mHair) == -1)
            messages.add(id, "Hair body part '" + npc.mHair + "' does not exist", "", CSMDoc::Message::Severity_Error);
        /// \todo Check gender, race and other body part stuff validity for the specific NPC
    }

    // Check inventory
    inventoryListCheck(npc.mInventory.mList, messages, id.toString());
 
    // Check that mentioned scripts exist
    scriptCheck<ESM::NPC>(npc, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::weaponCheck(
    int stage, const CSMWorld::RefIdDataContainer< ESM::Weapon >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Weapon& weapon = (dynamic_cast<const CSMWorld::Record<ESM::Weapon>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Weapon, weapon.mId);

    //TODO, It seems that this stuff for spellcasting is obligatory and In fact We should check if records are present
    if
    (   //THOSE ARE HARDCODED!
        !(weapon.mId == "VFX_Hands" ||
          weapon.mId == "VFX_Absorb" ||
          weapon.mId == "VFX_Reflect" ||
          weapon.mId == "VFX_DefaultBolt" ||
          //TODO I don't know how to get full list of effects :/
          //DANGER!, ACHTUNG! FIXME! The following is the list of the magical bolts, valid for Morrowind.esm. However those are not hardcoded.
          weapon.mId == "magic_bolt" ||
          weapon.mId == "shock_bolt" ||
          weapon.mId == "shield_bolt" ||
          weapon.mId == "VFX_DestructBolt" ||
          weapon.mId == "VFX_PoisonBolt" ||
          weapon.mId == "VFX_RestoreBolt" ||
          weapon.mId == "VFX_AlterationBolt" ||
          weapon.mId == "VFX_ConjureBolt" ||
          weapon.mId == "VFX_FrostBolt" ||
          weapon.mId == "VFX_MysticismBolt" ||
          weapon.mId == "VFX_IllusionBolt" ||
          weapon.mId == "VFX_Multiple2" ||
          weapon.mId == "VFX_Multiple3" ||
          weapon.mId == "VFX_Multiple4" ||
          weapon.mId == "VFX_Multiple5" ||
          weapon.mId == "VFX_Multiple6" ||
          weapon.mId == "VFX_Multiple7" ||
          weapon.mId == "VFX_Multiple8" ||
          weapon.mId == "VFX_Multiple9"))
    {
        inventoryItemCheck<ESM::Weapon>(weapon, messages, id.toString(), true);

        if (!(weapon.mData.mType == ESM::Weapon::MarksmanBow ||
                weapon.mData.mType == ESM::Weapon::MarksmanCrossbow ||
                weapon.mData.mType == ESM::Weapon::MarksmanThrown ||
                weapon.mData.mType == ESM::Weapon::Arrow ||
                weapon.mData.mType == ESM::Weapon::Bolt))
        {
            if (weapon.mData.mSlash[0] > weapon.mData.mSlash[1])
                messages.add(id, "Minimum slash damage higher than maximum", "", CSMDoc::Message::Severity_Warning);

            if (weapon.mData.mThrust[0] > weapon.mData.mThrust[1])
                messages.add(id, "Minimum thrust damage higher than maximum", "", CSMDoc::Message::Severity_Warning);
        }

        if (weapon.mData.mChop[0] > weapon.mData.mChop[1])
            messages.add(id, "Minimum chop damage higher than maximum", "", CSMDoc::Message::Severity_Warning);

        if (!(weapon.mData.mType == ESM::Weapon::Arrow ||
                weapon.mData.mType == ESM::Weapon::Bolt ||
                weapon.mData.mType == ESM::Weapon::MarksmanThrown))
        {
            //checking of health
            if (weapon.mData.mHealth == 0)
                messages.add(id, "Durability is equal to zero", "", CSMDoc::Message::Severity_Warning);

            if (weapon.mData.mReach < 0)
                messages.add(id, "Reach is negative", "", CSMDoc::Message::Severity_Error);
        }
    }

    // Check that mentioned scripts exist
    scriptCheck<ESM::Weapon>(weapon, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::probeCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Probe >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Probe& probe = (dynamic_cast<const CSMWorld::Record<ESM::Probe>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Probe, probe.mId);

    inventoryItemCheck<ESM::Probe>(probe, messages, id.toString());
    toolCheck<ESM::Probe>(probe, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Probe>(probe, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::repairCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::Repair >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Repair& repair = (dynamic_cast<const CSMWorld::Record<ESM::Repair>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Repair, repair.mId);

    inventoryItemCheck<ESM::Repair> (repair, messages, id.toString());
    toolCheck<ESM::Repair> (repair, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Repair>(repair, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::staticCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::Static >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Static& staticElement = (dynamic_cast<const CSMWorld::Record<ESM::Static>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Static, staticElement.mId);

    if (staticElement.mModel.empty())
        messages.add(id, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(staticElement.mModel) == -1)
        messages.add(id, "Model '" + staticElement.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);
}

//final check

void CSMTools::ReferenceableCheckStage::finalCheck (CSMDoc::Messages& messages)
{
    if (!mPlayerPresent)
        messages.add(CSMWorld::UniversalId::Type_Referenceables, "Player record is missing", "", CSMDoc::Message::Severity_SeriousError);
}

void CSMTools::ReferenceableCheckStage::inventoryListCheck(
    const std::vector<ESM::ContItem>& itemList, 
    CSMDoc::Messages& messages, 
    const std::string& id)
{
    for (size_t i = 0; i < itemList.size(); ++i)
    {
        std::string itemName = itemList[i].mItem;
        CSMWorld::RefIdData::LocalIndex localIndex = mReferencables.searchId(itemName);

        if (localIndex.first == -1)
            messages.add(id, "Item '" + itemName + "' does not exist", "", CSMDoc::Message::Severity_Error);
        else
        {
            // Needs to accommodate containers, creatures, and NPCs
            switch (localIndex.second)
            {
            case CSMWorld::UniversalId::Type_Potion:
            case CSMWorld::UniversalId::Type_Apparatus:
            case CSMWorld::UniversalId::Type_Armor:
            case CSMWorld::UniversalId::Type_Book:
            case CSMWorld::UniversalId::Type_Clothing:
            case CSMWorld::UniversalId::Type_Ingredient:
            case CSMWorld::UniversalId::Type_Light:
            case CSMWorld::UniversalId::Type_Lockpick:
            case CSMWorld::UniversalId::Type_Miscellaneous:
            case CSMWorld::UniversalId::Type_Probe:
            case CSMWorld::UniversalId::Type_Repair:
            case CSMWorld::UniversalId::Type_Weapon:
            case CSMWorld::UniversalId::Type_ItemLevelledList:
                break;
            default:
                messages.add(id, "'" + itemName + "' is not an item", "", CSMDoc::Message::Severity_Error);
            }
        }
    }
}

//Templates begins here

template<typename Item> void CSMTools::ReferenceableCheckStage::inventoryItemCheck (
    const Item& someItem, CSMDoc::Messages& messages, const std::string& someID, bool enchantable)
{
    if (someItem.mName.empty())
        messages.add(someID, "Name is missing", "", CSMDoc::Message::Severity_Error);

    //Checking for weight
    if (someItem.mData.mWeight < 0)
        messages.add(someID, "Weight is negative", "", CSMDoc::Message::Severity_Error);

    //Checking for value
    if (someItem.mData.mValue < 0)
        messages.add(someID, "Value is negative", "", CSMDoc::Message::Severity_Error);

    //checking for model
    if (someItem.mModel.empty())
        messages.add(someID, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(someItem.mModel) == -1)
        messages.add(someID, "Model '" + someItem.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    //checking for icon
    if (someItem.mIcon.empty())
        messages.add(someID, "Icon is missing", "", CSMDoc::Message::Severity_Error);
    else if (mIcons.searchId(someItem.mIcon) == -1)
    {
        std::string ddsIcon = someItem.mIcon;
        if (!(Misc::ResourceHelpers::changeExtensionToDds(ddsIcon) && mIcons.searchId(ddsIcon) != -1))
            messages.add(someID, "Icon '" + someItem.mIcon + "' does not exist", "", CSMDoc::Message::Severity_Error);
    }

    if (enchantable && someItem.mData.mEnchant < 0)
        messages.add(someID, "Enchantment points number is negative", "", CSMDoc::Message::Severity_Error);
}

template<typename Item> void CSMTools::ReferenceableCheckStage::inventoryItemCheck (
    const Item& someItem, CSMDoc::Messages& messages, const std::string& someID)
{
    if (someItem.mName.empty())
        messages.add(someID, "Name is missing", "", CSMDoc::Message::Severity_Error);

    //Checking for weight
    if (someItem.mData.mWeight < 0)
        messages.add(someID, "Weight is negative", "", CSMDoc::Message::Severity_Error);

    //Checking for value
    if (someItem.mData.mValue < 0)
        messages.add(someID, "Value is negative", "", CSMDoc::Message::Severity_Error);

    //checking for model
    if (someItem.mModel.empty())
        messages.add(someID, "Model is missing", "", CSMDoc::Message::Severity_Error);
    else if (mModels.searchId(someItem.mModel) == -1)
        messages.add(someID, "Model '" + someItem.mModel + "' does not exist", "", CSMDoc::Message::Severity_Error);

    //checking for icon
    if (someItem.mIcon.empty())
        messages.add(someID, "Icon is missing", "", CSMDoc::Message::Severity_Error);
    else if (mIcons.searchId(someItem.mIcon) == -1)
    {
        std::string ddsIcon = someItem.mIcon;
        if (!(Misc::ResourceHelpers::changeExtensionToDds(ddsIcon) && mIcons.searchId(ddsIcon) != -1))
            messages.add(someID, "Icon '" + someItem.mIcon + "' does not exist", "", CSMDoc::Message::Severity_Error);
    }
}

template<typename Tool> void CSMTools::ReferenceableCheckStage::toolCheck (
    const Tool& someTool, CSMDoc::Messages& messages, const std::string& someID, bool canBeBroken)
{
    if (someTool.mData.mQuality <= 0)
        messages.add(someID, "Quality is non-positive", "", CSMDoc::Message::Severity_Error);

    if (canBeBroken && someTool.mData.mUses<=0)
        messages.add(someID, "Number of uses is non-positive", "", CSMDoc::Message::Severity_Error);
}

template<typename Tool> void CSMTools::ReferenceableCheckStage::toolCheck (
    const Tool& someTool, CSMDoc::Messages& messages, const std::string& someID)
{
    if (someTool.mData.mQuality <= 0)
        messages.add(someID, "Quality is non-positive", "", CSMDoc::Message::Severity_Error);
}

template<typename List> void CSMTools::ReferenceableCheckStage::listCheck (
    const List& someList, CSMDoc::Messages& messages, const std::string& someID)
{
    if (someList.mChanceNone > 100)
    {
        messages.add(someID, "Chance that no object is used is over 100 percent", "", CSMDoc::Message::Severity_Warning);
    }

    for (unsigned i = 0; i < someList.mList.size(); ++i)
    {
        if (mReferencables.searchId(someList.mList[i].mId).first == -1)
            messages.add(someID, "Object '" + someList.mList[i].mId + "' does not exist", "", CSMDoc::Message::Severity_Error);

        if (someList.mList[i].mLevel < 1)
            messages.add(someID, "Level of item '" + someList.mList[i].mId + "' is non-positive", "", CSMDoc::Message::Severity_Error);
    }
}

template<typename Tool> void CSMTools::ReferenceableCheckStage::scriptCheck (
    const Tool& someTool, CSMDoc::Messages& messages, const std::string& someID)
{
    if (!someTool.mScript.empty())
    {
        if (mScripts.searchId(someTool.mScript) == -1)
            messages.add(someID, "Script '" + someTool.mScript + "' does not exist", "", CSMDoc::Message::Severity_Error);
    }
}
