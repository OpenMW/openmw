
#include "refidcollection.hpp"

#include <stdexcept>
#include <memory>

#include <components/esm/esmreader.hpp>

#include "refidadapter.hpp"
#include "refidadapterimp.hpp"
#include "columns.hpp"

CSMWorld::RefIdColumn::RefIdColumn (int columnId, Display displayType, int flag,
    bool editable, bool userEditable)
: ColumnBase (columnId, displayType, flag), mEditable (editable), mUserEditable (userEditable)
{}

bool CSMWorld::RefIdColumn::isEditable() const
{
    return mEditable;
}

bool CSMWorld::RefIdColumn::isUserEditable() const
{
    return mUserEditable;
}


const CSMWorld::RefIdAdapter& CSMWorld::RefIdCollection::findAdaptor (UniversalId::Type type) const
{
    std::map<UniversalId::Type, RefIdAdapter *>::const_iterator iter = mAdapters.find (type);

    if (iter==mAdapters.end())
        throw std::logic_error ("unsupported type in RefIdCollection");

    return *iter->second;
}

CSMWorld::RefIdCollection::RefIdCollection()
{
    BaseColumns baseColumns;

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Id, ColumnBase::Display_String,
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

    EnchantableColumns enchantableColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Enchantment, ColumnBase::Display_String));
    enchantableColumns.mEnchantment = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_EnchantmentPoints, ColumnBase::Display_Integer));
    enchantableColumns.mEnchantmentPoints = &mColumns.back();

    ToolColumns toolsColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Quality, ColumnBase::Display_Float));
    toolsColumns.mQuality = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Charges, ColumnBase::Display_Integer));
    toolsColumns.mUses = &mColumns.back();

    ActorColumns actorsColumns (nameColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Ai, ColumnBase::Display_Boolean));
    actorsColumns.mHasAi = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiHello, ColumnBase::Display_Integer));
    actorsColumns.mHello = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiFlee, ColumnBase::Display_Integer));
    actorsColumns.mFlee = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiFight, ColumnBase::Display_Integer));
    actorsColumns.mFight = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_AiAlarm, ColumnBase::Display_Integer));
    actorsColumns.mAlarm = &mColumns.back();

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

    mColumns.push_back (RefIdColumn (Columns::ColumnId_AutoCalc, ColumnBase::Display_Boolean));
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

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Scroll, ColumnBase::Display_Boolean));
    const RefIdColumn *scroll = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Attribute, ColumnBase::Display_Attribute));
    const RefIdColumn *attribute = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_ClothingType, ColumnBase::Display_ClothingType));
    const RefIdColumn *clothingType = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_WeightCapacity, ColumnBase::Display_Float));
    const RefIdColumn *weightCapacity = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_OrganicContainer, ColumnBase::Display_Boolean));
    const RefIdColumn *organic = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Respawn, ColumnBase::Display_Boolean));
    const RefIdColumn *respawn = &mColumns.back();

    CreatureColumns creatureColumns (actorsColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_CreatureType, ColumnBase::Display_CreatureType));
    creatureColumns.mType = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_SoulPoints, ColumnBase::Display_Integer));
    creatureColumns.mSoul = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_Scale, ColumnBase::Display_Float));
    creatureColumns.mScale = &mColumns.back();
    mColumns.push_back (RefIdColumn (Columns::ColumnId_OriginalCreature, ColumnBase::Display_String));
    creatureColumns.mOriginal = &mColumns.back();
    mColumns.push_back (
        RefIdColumn (Columns::ColumnId_CombatState, ColumnBase::Display_Integer));
    creatureColumns.mCombat = &mColumns.back();
    mColumns.push_back (
        RefIdColumn (Columns::ColumnId_MagicState, ColumnBase::Display_Integer));
    creatureColumns.mMagic = &mColumns.back();
    mColumns.push_back (
        RefIdColumn (Columns::ColumnId_StealthState, ColumnBase::Display_Integer));
    creatureColumns.mStealth = &mColumns.back();

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sCreatureFlagTable[] =
    {
        { Columns::ColumnId_Biped, ESM::Creature::Bipedal },
        { Columns::ColumnId_HasWeapon, ESM::Creature::Weapon },
        { Columns::ColumnId_NoMovement, ESM::Creature::None },
        { Columns::ColumnId_Swims, ESM::Creature::Swims },
        { Columns::ColumnId_Flies, ESM::Creature::Flies },
        { Columns::ColumnId_Walks, ESM::Creature::Walks },
        { Columns::ColumnId_Essential, ESM::Creature::Essential },
        { Columns::ColumnId_SkeletonBlood, ESM::Creature::Skeleton },
        { Columns::ColumnId_MetalBlood, ESM::Creature::Metal },
        { -1, 0 }
    };

    // for re-use in NPC records
    const RefIdColumn *essential = 0;
    const RefIdColumn *skeletonBlood = 0;
    const RefIdColumn *metalBlood = 0;

    for (int i=0; sCreatureFlagTable[i].mName!=-1; ++i)
    {
        mColumns.push_back (RefIdColumn (sCreatureFlagTable[i].mName, ColumnBase::Display_Boolean));
        creatureColumns.mFlags.insert (std::make_pair (&mColumns.back(), sCreatureFlagTable[i].mFlag));

        switch (sCreatureFlagTable[i].mFlag)
        {
            case ESM::Creature::Essential: essential = &mColumns.back(); break;
            case ESM::Creature::Skeleton: skeletonBlood = &mColumns.back(); break;
            case ESM::Creature::Metal: metalBlood = &mColumns.back(); break;
        }
    }

    creatureColumns.mFlags.insert (std::make_pair (respawn, ESM::Creature::Respawn));

    mColumns.push_back (RefIdColumn (Columns::ColumnId_OpenSound, ColumnBase::Display_Sound));
    const RefIdColumn *openSound = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_CloseSound, ColumnBase::Display_Sound));
    const RefIdColumn *closeSound = &mColumns.back();

    LightColumns lightColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Duration, ColumnBase::Display_Integer));
    lightColumns.mTime = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Radius, ColumnBase::Display_Integer));
    lightColumns.mRadius = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Colour, ColumnBase::Display_Integer));
    lightColumns.mColor = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Sound, ColumnBase::Display_Sound));
    lightColumns.mSound = &mColumns.back();

    static const struct
    {
        int mName;
        unsigned int mFlag;
    } sLightFlagTable[] =
    {
        { Columns::ColumnId_Dynamic, ESM::Light::Dynamic },
        { Columns::ColumnId_Portable, ESM::Light::Carry },
        { Columns::ColumnId_NegativeLight, ESM::Light::Negative },
        { Columns::ColumnId_Flickering, ESM::Light::Flicker },
        { Columns::ColumnId_SlowFlickering, ESM::Light::Flicker },
        { Columns::ColumnId_Pulsing, ESM::Light::Pulse },
        { Columns::ColumnId_SlowPulsing, ESM::Light::PulseSlow },
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

    mColumns.push_back (RefIdColumn (Columns::Columnid_Hair, ColumnBase::Display_String));
    npcColumns.mHair = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Head, ColumnBase::Display_String));
    npcColumns.mHead = &mColumns.back();

    mColumns.push_back (RefIdColumn (Columns::ColumnId_Female, ColumnBase::Display_Boolean));
    npcColumns.mFlags.insert (std::make_pair (&mColumns.back(), ESM::NPC::Female));

    npcColumns.mFlags.insert (std::make_pair (essential, ESM::NPC::Essential));

    npcColumns.mFlags.insert (std::make_pair (respawn, ESM::NPC::Respawn));

    npcColumns.mFlags.insert (std::make_pair (autoCalc, ESM::NPC::Autocalc));

    npcColumns.mFlags.insert (std::make_pair (skeletonBlood, ESM::NPC::Skeleton));

    npcColumns.mFlags.insert (std::make_pair (metalBlood, ESM::NPC::Metal));

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

    mAdapters.insert (std::make_pair (UniversalId::Type_Activator,
        new NameRefIdAdapter<ESM::Activator> (UniversalId::Type_Activator, nameColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Potion,
        new PotionRefIdAdapter (inventoryColumns, autoCalc)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Apparatus,
        new ApparatusRefIdAdapter (inventoryColumns, apparatusType, toolsColumns.mQuality)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Armor,
        new ArmorRefIdAdapter (enchantableColumns, armorType, health, armor)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Book,
        new BookRefIdAdapter (enchantableColumns, scroll, attribute)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Clothing,
        new ClothingRefIdAdapter (enchantableColumns, clothingType)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Container,
        new ContainerRefIdAdapter (nameColumns, weightCapacity, organic, respawn)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Creature,
        new CreatureRefIdAdapter (creatureColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Door,
        new DoorRefIdAdapter (nameColumns, openSound, closeSound)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Ingredient,
        new InventoryRefIdAdapter<ESM::Ingredient> (UniversalId::Type_Ingredient, inventoryColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_CreatureLevelledList,
        new BaseRefIdAdapter<ESM::CreatureLevList> (
        UniversalId::Type_CreatureLevelledList, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_ItemLevelledList,
        new BaseRefIdAdapter<ESM::ItemLevList> (UniversalId::Type_ItemLevelledList, baseColumns)));
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

    const RefIdAdapter& adaptor = findAdaptor (localIndex.second);

    return adaptor.getData (&mColumns.at (column), mData, localIndex.first);
}

void CSMWorld::RefIdCollection::setData (int index, int column, const QVariant& data)
{
    RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (index);

    const RefIdAdapter& adaptor = findAdaptor (localIndex.second);

    adaptor.setData (&mColumns.at (column), mData, localIndex.first, data);
}

void CSMWorld::RefIdCollection::removeRows (int index, int count)
{
    mData.erase (index, count);
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
        std::auto_ptr<RecordBase> newRecord(mData.getRecord(mData.searchId(origin)).clone());
        newRecord->mState = RecordBase::State_ModifiedOnly;
        mAdapters.find(type)->second->setId(*newRecord, destination);
        mData.insertRecord(*newRecord, type, destination);
}

void CSMWorld::RefIdCollection::appendRecord (const RecordBase& record,
    UniversalId::Type type)
{
    std::string id = findAdaptor (type).getId (record);

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
    std::string id = reader.getHNOString ("NAME");

    int index = searchId (id);

    if (reader.isNextSub ("DELE"))
    {
        reader.skipRecord();

        if (index==-1)
        {
            // deleting a record that does not exist

            // ignore it for now

            /// \todo report the problem to the user
        }
        else if (base)
        {
            mData.erase (index, 1);
        }
        else
        {
            mData.getRecord (mData.globalToLocalIndex (index)).mState = RecordBase::State_Deleted;
        }
    }
    else
    {
        if (index==-1)
        {
            // new record
            int index = mData.getAppendIndex (type);
            mData.appendRecord (type, id, base);

            RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (index);

            mData.load (localIndex, reader, base);

            mData.getRecord (localIndex).mState =
                base ? RecordBase::State_BaseOnly : RecordBase::State_ModifiedOnly;
        }
        else
        {
            // old record
            RefIdData::LocalIndex localIndex = mData.globalToLocalIndex (index);

            if (!base)
                if (mData.getRecord (localIndex).mState==RecordBase::State_Erased)
                    throw std::logic_error ("attempt to access a deleted record");

            mData.load (localIndex, reader, base);

            if (!base)
                mData.getRecord (localIndex).mState = RecordBase::State_Modified;
        }
    }
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

