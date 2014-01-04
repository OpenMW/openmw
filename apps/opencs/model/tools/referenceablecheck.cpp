#include "referenceablecheck.hpp"

#include <sstream>
#include <map>
#include <cassert>
#include <boost/graph/graph_concepts.hpp>

#include "../world/record.hpp"

#include "../world/universalid.hpp"
#include <components/esm/loadmgef.hpp>

CSMTools::ReferenceableCheckStage::ReferenceableCheckStage(
    const CSMWorld::RefIdData& referenceable, const CSMWorld::IdCollection<ESM::Race >& races,
    const CSMWorld::IdCollection<ESM::Class>& classes,
    const CSMWorld::IdCollection<ESM::Faction>& faction)
    :
    mReferencables(referenceable),
    mClasses(classes),
    mRaces(races),
    mFactions(faction)
{
}

void CSMTools::ReferenceableCheckStage::perform(int stage, std::vector< std::string >& messages)
{
    //Checks for books, than, when stage is above mBooksSize goes to other checks, with (stage - PrevSum) as stage.
    const int BookSize(mReferencables.getBooks().getSize());

    if (stage < BookSize)
    {
        bookCheck(stage, mReferencables.getBooks(), messages);
        return;
    }

    stage -= BookSize;

    const int ActivatorSize(mReferencables.getActivators().getSize());

    if (stage < ActivatorSize)
    {
        activatorCheck(stage, mReferencables.getActivators(), messages);
        return;
    }

    stage -= ActivatorSize;

    const int PotionSize(mReferencables.getActivators().getSize());

    if (stage < PotionSize)
    {
        potionCheck(stage, mReferencables.getPotions(), messages);
        return;
    }

    stage -= PotionSize;

    const int ApparatusSize(mReferencables.getApparati().getSize());

    if (stage < ApparatusSize)
    {
        apparatusCheck(stage, mReferencables.getApparati(), messages);
        return;
    }

    stage -= ApparatusSize;

    const int ArmorSize(mReferencables.getArmors().getSize());

    if (stage < ArmorSize)
    {
        armorCheck(stage, mReferencables.getArmors(), messages);
        return;
    }

    stage -= ArmorSize;

    const int ClothingSize(mReferencables.getClothing().getSize());

    if (stage < ClothingSize)
    {
        clothingCheck(stage, mReferencables.getClothing(), messages);
        return;
    }

    stage -= ClothingSize;

    const int ContainerSize(mReferencables.getContainers().getSize());

    if (stage < ContainerSize)
    {
        containerCheck(stage, mReferencables.getContainers(), messages);
        return;
    }

    stage -= ContainerSize;

    const int DoorSize(mReferencables.getDoors().getSize());

    if (stage < DoorSize)
    {
        doorCheck(stage, mReferencables.getDoors(), messages);
        return;
    }

    stage -= DoorSize;

    const int IngredientSize(mReferencables.getIngredients().getSize());

    if (stage < IngredientSize)
    {
        ingredientCheck(stage, mReferencables.getIngredients(), messages);
        return;
    }

    stage -= IngredientSize;

    const int CreatureLevListSize(mReferencables.getCreatureLevelledLists().getSize());

    if (stage < CreatureLevListSize)
    {
        creaturesLevListCheck(stage, mReferencables.getCreatureLevelledLists(), messages);
        return;
    }

    stage -= CreatureLevListSize;

    const int ItemLevelledListSize(mReferencables.getItemLevelledList().getSize());

    if (stage < ItemLevelledListSize)
    {
        itemLevelledListCheck(stage, mReferencables.getItemLevelledList(), messages);
        return;
    }

    stage -= ItemLevelledListSize;

    const int LightSize(mReferencables.getLights().getSize());

    if (stage < LightSize)
    {
        lightCheck(stage, mReferencables.getLights(), messages);
        return;
    }

    stage -= LightSize;

    const int LockpickSize(mReferencables.getLocpicks().getSize());

    if (stage < LockpickSize)
    {
        lockpickCheck(stage, mReferencables.getLocpicks(), messages);
        return;
    }

    stage -= LockpickSize;

    const int MiscSize(mReferencables.getMiscellaneous().getSize());

    if (stage < MiscSize)
    {
        miscCheck(stage, mReferencables.getMiscellaneous(), messages);
        return;
    }

    stage -= MiscSize;

    const int NPCSize(mReferencables.getNPCs().getSize());

    if (stage < NPCSize)
    {
        npcCheck(stage, mReferencables.getNPCs(), messages);
        return;
    }

    stage -= NPCSize;

    const int WeaponSize(mReferencables.getWeapons().getSize());

    if (stage < WeaponSize)
    {
        weaponCheck(stage, mReferencables.getWeapons(), messages);
        return;
    }

    stage -= WeaponSize;

    const int ProbeSize(mReferencables.getProbes().getSize());

    if (stage < ProbeSize)
    {
        probeCheck(stage, mReferencables.getProbes(), messages);
        return;
    }

    stage -= ProbeSize;

    const int RepairSize(mReferencables.getRepairs().getSize());

    if (stage < RepairSize)
    {
        repairCheck(stage, mReferencables.getRepairs(), messages);
        return;
    }

    stage -= RepairSize;

    const int StaticSize(mReferencables.getStatics().getSize());

    if (stage < StaticSize)
    {
        staticCheck(stage, mReferencables.getStatics(), messages);
        return;
    }
}

int CSMTools::ReferenceableCheckStage::setup()
{
    return mReferencables.getSize();
}

void CSMTools::ReferenceableCheckStage::bookCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Book >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Book& Book = (dynamic_cast<const CSMWorld::Record<ESM::Book>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Book, Book.mId);

    inventoryItemCheck<ESM::Book>(Book, messages, id.toString(), true);
}

void CSMTools::ReferenceableCheckStage::activatorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Activator >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Activator& Activator = (dynamic_cast<const CSMWorld::Record<ESM::Activator>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Activator, Activator.mId);

    //Checking for model, IIRC all activators should have a model
    if (Activator.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Activator.mId + " has no model");
    }
}

void CSMTools::ReferenceableCheckStage::potionCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Potion >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Potion& Potion = (dynamic_cast<const CSMWorld::Record<ESM::Potion>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Potion, Potion.mId);

    inventoryItemCheck<ESM::Potion>(Potion, messages, id.toString());
    //IIRC potion can have empty effects list just fine.
}


void CSMTools::ReferenceableCheckStage::apparatusCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Apparatus >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Apparatus& Apparatus = (dynamic_cast<const CSMWorld::Record<ESM::Apparatus>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Apparatus, Apparatus.mId);

    inventoryItemCheck<ESM::Apparatus>(Apparatus, messages, id.toString());

    //checking for quality, 0 → apparatus is basicly useless, any negative → apparatus is harmfull instead of helpfull
    toolCheck<ESM::Apparatus>(Apparatus, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::armorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Armor >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Armor& Armor = (dynamic_cast<const CSMWorld::Record<ESM::Armor>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Armor, Armor.mId);

    inventoryItemCheck<ESM::Armor>(Armor, messages, id.toString(), true);

    //checking for armor class, armor should have poistive armor class, but 0 is considered legal
    if (Armor.mData.mArmor < 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has negative armor class");
    }

    //checking for health. Only positive numbers are allowed, or 0 is illegal
    if (Armor.mData.mHealth <= 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has non positive health");
    }
}

void CSMTools::ReferenceableCheckStage::clothingCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Clothing >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Clothing& Clothing = (dynamic_cast<const CSMWorld::Record<ESM::Clothing>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Clothing, Clothing.mId);
    inventoryItemCheck<ESM::Clothing>(Clothing, messages, id.toString(), true);
}

void CSMTools::ReferenceableCheckStage::containerCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Container >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Container& Container = (dynamic_cast<const CSMWorld::Record<ESM::Container>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Container, Container.mId);

    //Checking for model, IIRC all containers should have a model
    if (Container.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Container.mId + " has no model");
    }

    //Checking for capacity (weight)
    if (Container.mWeight < 0) //0 is allowed
    {
        messages.push_back(id.toString() + "|" + Container.mId + " has negative weight (capacity)");
    }

    //checking for name
    if (Container.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Container.mId + " has an empty name");
    }
}

void CSMTools::ReferenceableCheckStage::creatureCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Creature >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Creature& Creature = (dynamic_cast<const CSMWorld::Record<ESM::Creature>&>(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Creature, Creature.mId);

    if (Creature.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has no model");
    }

    if (Creature.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has an empty name");
    }

    //stats checks
    if (Creature.mData.mLevel < 1)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has non-postive level");
    }

    if (Creature.mData.mStrength < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative strength");
    }

    if (Creature.mData.mIntelligence < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative intelligence");
    }

    if (Creature.mData.mWillpower < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative willpower");
    }

    if (Creature.mData.mAgility < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative agility");
    }

    if (Creature.mData.mSpeed < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative speed");
    }

    if (Creature.mData.mEndurance < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative endurance");
    }

    if (Creature.mData.mPersonality < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative personality");
    }

    if (Creature.mData.mLuck < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative luck");
    }

    if (Creature.mData.mHealth < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative health");
    }

    if (Creature.mData.mSoul < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative soul value");
    }

    for (int i = 0; i < 6; ++i)
    {
        if (Creature.mData.mAttack[i] < 0)
        {
            messages.push_back(id.toString() + "|" + Creature.mId + " has negative attack strength");
            break;
        }
    }

    //TODO, find meaning of other values
    if (Creature.mData.mGold < 0) //It seems that this is for gold in merchant creatures
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative gold ");
    }
}

void CSMTools::ReferenceableCheckStage::doorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Door >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Door& Door = (dynamic_cast<const CSMWorld::Record<ESM::Door>&>(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Door, Door.mId);

    //usual, name or model
    if (Door.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Door.mId + " has an empty name");
    }

    if (Door.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Door.mId + " has no model");
    }

    //TODO, check what static unsigned int sRecordId; is for
}

void CSMTools::ReferenceableCheckStage::ingredientCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Ingredient >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Ingredient& Ingredient = (dynamic_cast<const CSMWorld::Record<ESM::Ingredient>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Ingredient, Ingredient.mId);

    inventoryItemCheck<ESM::Ingredient>(Ingredient, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::creaturesLevListCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::CreatureLevList >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::CreatureLevList& CreatureLevList = (dynamic_cast<const CSMWorld::Record<ESM::CreatureLevList>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_CreatureLevelledList, CreatureLevList.mId); //CreatureLevList but Type_CreatureLevelledList :/


    listCheck<ESM::CreatureLevList>(CreatureLevList, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::itemLevelledListCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::ItemLevList >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::ItemLevList& ItemLevList = (dynamic_cast<const CSMWorld::Record<ESM::ItemLevList>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_ItemLevelledList, ItemLevList.mId);

    listCheck<ESM::ItemLevList>(ItemLevList, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::lightCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Light >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Light& Light = (dynamic_cast<const CSMWorld::Record<ESM::Light>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Light, Light.mId);

    if (Light.mData.mRadius < 0)
    {
        messages.push_back(id.toString() + "|" + Light.mId + " has negative light radius");
    }

    if (Light.mData.mFlags & ESM::Light::Carry)
    {
        if (Light.mIcon.empty()) //Needs to be checked with carrable flag
        {
            inventoryItemCheck<ESM::Light>(Light, messages, id.toString());

            if (Light.mData.mTime == 0)
            {
                messages.push_back(id.toString() + "|" + Light.mId + " has zero duration");
            }
        }
    }
}

void CSMTools::ReferenceableCheckStage::lockpickCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Lockpick >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Lockpick& Lockpick = (dynamic_cast<const CSMWorld::Record<ESM::Lockpick>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Lockpick, Lockpick.mId);

    inventoryItemCheck<ESM::Lockpick>(Lockpick, messages, id.toString());

    toolCheck<ESM::Lockpick>(Lockpick, messages, id.toString(), true);
}

void CSMTools::ReferenceableCheckStage::miscCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Miscellaneous >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Miscellaneous& Miscellaneous = (dynamic_cast<const CSMWorld::Record<ESM::Miscellaneous>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Miscellaneous, Miscellaneous.mId);

    inventoryItemCheck<ESM::Miscellaneous>(Miscellaneous, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::npcCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::NPC >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::NPC& NPC = (dynamic_cast<const CSMWorld::Record<ESM::NPC>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Npc, NPC.mId);

    short level(NPC.mNpdt52.mLevel);
    char Disposition(NPC.mNpdt52.mDisposition);
    char Reputation(NPC.mNpdt52.mReputation);
    char Rank(NPC.mNpdt52.mRank);
    //Don't know what unknown is for
    int Gold(NPC.mNpdt52.mGold);

    if (NPC.mNpdtType == 12) //12 = autocalculated
    {
        if ((NPC.mFlags & 0x0008) == 0) //0x0008 = autocalculated flag
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " mNpdtType or flags mismatch!"); //should not happend?
            return;
        }

        level = NPC.mNpdt12.mLevel;
        Disposition = NPC.mNpdt12.mDisposition;
        Reputation = NPC.mNpdt12.mReputation;
        Rank = NPC.mNpdt12.mRank;
        Gold = NPC.mNpdt12.mGold;
    }
    else
    {
        if (NPC.mNpdt52.mMana < 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " mana has negative value");
        }

        if (NPC.mNpdt52.mFatigue < 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " fatigue has negative value");
        }

        if (NPC.mNpdt52.mAgility == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " agility has zero value");
        }

        if (NPC.mNpdt52.mEndurance == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " endurance has zero value");
        }

        if (NPC.mNpdt52.mIntelligence == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " intelligence has zero value");
        }

        if (NPC.mNpdt52.mLuck == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " luck has zero value");
        }

        if (NPC.mNpdt52.mPersonality == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " personality has zero value");
        }

        if (NPC.mNpdt52.mStrength == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " strength has zero value");
        }

        if (NPC.mNpdt52.mSpeed == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " speed has zero value");
        }

        if (NPC.mNpdt52.mWillpower == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " willpower has zero value");
        }
    }

    if (level < 1)
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " level is non positive");
    }

    if (Gold < 0)
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " gold has negative value");
    }

    if (NPC.mName.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has any empty name");
    }

    if (NPC.mClass.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has any empty class");
    }
    else //checking if there is such class
    {
        if (mClasses.searchId(NPC.mClass) == -1)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " has invalid class");
        }
    }

    if (NPC.mRace.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has any empty race");
    }
    else //checking if there is a such race
    {
        bool nosuchrace(true);

        for (int i = 0; i < mRaces.getSize(); ++i)
        {
            if (dynamic_cast<const ESM::Race&>(mRaces.getRecord(i).get()).mName == NPC.mRace) //mId in class, mName for race. Stupid.
            {
                nosuchrace = false;
                break;
            }
        }

        if (nosuchrace)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " has invalid race");
        }
    }

    if (Disposition < 0)
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has negative disposition");
    }

    if (Reputation < 0) //It seems that no character in Morrowind.esm have negative reputation. I'm assuming that negative reputation is invalid
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has negative reputation");
    }

    if (NPC.mFaction.empty() == false)
    {
        if (Rank < 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " has negative rank");
        }

        if (mFactions.searchId(NPC.mFaction) == -1)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " has invalid faction");
        }
    }

    if (NPC.mHead.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has no head");
    }

    if (NPC.mHair.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has no hair");
    }

    //TODO: reputation, Disposition, rank, everything else
}

void CSMTools::ReferenceableCheckStage::weaponCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Weapon >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Weapon& Weapon = (dynamic_cast<const CSMWorld::Record<ESM::Weapon>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Weapon, Weapon.mId);

    //TODO, It seems that this stuff for spellcasting is obligatory and In fact We should check if records are present
    if
    (
        //THOSE ARE HARDCODED!
        Weapon.mId != "VFX_Hands"
        && Weapon.mId != "VFX_Absorb"
        && Weapon.mId != "VFX_Reflect"
        && Weapon.mId != "VFX_DefaultBolt"
        //TODO I don't know how to get full list of effects :/
    )
    {
        inventoryItemCheck<ESM::Weapon>(Weapon, messages, id.toString(), true);

        if (!(Weapon.mData.mType == ESM::Weapon::MarksmanBow or Weapon.mData.mType == ESM::Weapon::MarksmanCrossbow or Weapon.mData.mType == ESM::Weapon::MarksmanThrown or Weapon.mData.mType == ESM::Weapon::Arrow or Weapon.mData.mType == ESM::Weapon::Bolt))
        {
            if (Weapon.mData.mSlash[0] > Weapon.mData.mSlash[1])
            {
                messages.push_back(id.toString() + "|" + Weapon.mId + " has minimum slash damage higher than maximum");
            }

            if (Weapon.mData.mThrust[0] > Weapon.mData.mThrust[1])
            {
                messages.push_back(id.toString() + "|" + Weapon.mId + " has minimum thrust damage higher than maximum");
            }
        }

        if (Weapon.mData.mChop[0] > Weapon.mData.mChop[1])
        {
            messages.push_back(id.toString() + "|" + Weapon.mId + " has minimum chop damage higher than maximum");
        }

        if (!(Weapon.mData.mType == ESM::Weapon::Arrow or Weapon.mData.mType == ESM::Weapon::Bolt or Weapon.mData.mType == ESM::Weapon::MarksmanThrown))
        {
            //checking of health
            if (Weapon.mData.mHealth <= 0)
            {
                messages.push_back(id.toString() + "|" + Weapon.mId + " has non-positivie health");
            }

            if (Weapon.mData.mReach < 0)
            {
                messages.push_back(id.toString() + "|" + Weapon.mId + " has negative reach");
            }
        }
    }
}

void CSMTools::ReferenceableCheckStage::probeCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Probe >& records,
    std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Probe& Probe = (dynamic_cast<const CSMWorld::Record<ESM::Probe>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Probe, Probe.mId);

    inventoryItemCheck<ESM::Probe>(Probe, messages, id.toString());
    toolCheck<ESM::Probe>(Probe, messages, id.toString(), true);
}

void CSMTools::ReferenceableCheckStage::repairCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Repair >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Repair& Repair = (dynamic_cast<const CSMWorld::Record<ESM::Repair>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Repair, Repair.mId);

    inventoryItemCheck<ESM::Repair>(Repair, messages, id.toString());
    toolCheck<ESM::Repair>(Repair, messages, id.toString(), true);
}

void CSMTools::ReferenceableCheckStage::staticCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Static >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Static& Static = (dynamic_cast<const CSMWorld::Record<ESM::Static>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Static, Static.mId);

    if (Static.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Static.mId + " has no model");
    }
}


//Templates begins here

template<typename ITEM> void CSMTools::ReferenceableCheckStage::inventoryItemCheck(
    const ITEM& someitem,
    std::vector< std::string >& messages,
    const std::string& someid, bool enchantable)
{
    if (someitem.mName.empty())
    {
        messages.push_back(someid + "|" + someitem.mId + " has an empty name");
    }

    //Checking for weight
    if (someitem.mData.mWeight < 0)
    {
        messages.push_back(someid + "|" + someitem.mId + " has negative weight");
    }

    //Checking for value
    if (someitem.mData.mValue < 0)
    {
        messages.push_back(someid + "|" + someitem.mId + " has negative value");
    }

//checking for model
    if (someitem.mModel.empty())
    {
        messages.push_back(someid + "|" + someitem.mId + " has no model");
    }

    //checking for icon
    if (someitem.mIcon.empty())
    {
        messages.push_back(someid + "|" + someitem.mId + " has no icon");
    }

    if (enchantable)
    {
        if (someitem.mData.mEnchant < 0)
        {
            messages.push_back(someid + "|" + someitem.mId + " has negative enchantment");
        }
    }
}

template<typename ITEM> void CSMTools::ReferenceableCheckStage::inventoryItemCheck(
    const ITEM& someitem,
    std::vector< std::string >& messages,
    const std::string& someid)
{
    if (someitem.mName.empty())
    {
        messages.push_back(someid + "|" + someitem.mId + " has an empty name");
    }

    //Checking for weight
    if (someitem.mData.mWeight < 0)
    {
        messages.push_back(someid + "|" + someitem.mId + " has negative weight");
    }

    //Checking for value
    if (someitem.mData.mValue < 0)
    {
        messages.push_back(someid + "|" + someitem.mId + " has negative value");
    }

//checking for model
    if (someitem.mModel.empty())
    {
        messages.push_back(someid + "|" + someitem.mId + " has no model");
    }

    //checking for icon
    if (someitem.mIcon.empty())
    {
        messages.push_back(someid + "|" + someitem.mId + " has no icon");
    }
}

template<typename TOOL> void CSMTools::ReferenceableCheckStage::toolCheck(
    const TOOL& sometool,
    std::vector< std::string >& messages,
    const std::string& someid, bool canbebroken)
{
    if (sometool.mData.mQuality <= 0)
    {
        messages.push_back(someid + "|" + sometool.mId + " has non-positive quality");
    }

    if (canbebroken)
    {
        if (sometool.mData.mUses <= 0)
        {
            messages.push_back(someid + "|" + sometool.mId + " has non-positive uses count");
        }
    }
}

template<typename TOOL> void CSMTools::ReferenceableCheckStage::toolCheck(
    const TOOL& sometool,
    std::vector< std::string >& messages,
    const std::string& someid)
{
    if (sometool.mData.mQuality <= 0)
    {
        messages.push_back(someid + "|" + sometool.mId + " has non-positive quality");
    }
}

template<typename LIST> void CSMTools::ReferenceableCheckStage::listCheck(
    const LIST& somelist,
    std::vector< std::string >& messages,
    const std::string& someid)
{
    for (unsigned i = 0; i < somelist.mList.size(); ++i)
    {
        if (somelist.mList[i].mId.empty())
        {
            messages.push_back(someid + "|" + somelist.mId + " contains item with empty Id");
        }

        if (somelist.mList[i].mLevel < 1)
        {
            messages.push_back(someid + "|" + somelist.mId + " contains item with non-positive level");
        }
    }
}
// kate: indent-mode cstyle; indent-width 4; replace-tabs on; 
