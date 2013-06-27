
#include "refidcollection.hpp"

#include <stdexcept>

#include "refidadapter.hpp"
#include "refidadapterimp.hpp"

CSMWorld::RefIdColumn::RefIdColumn (const std::string& title, Display displayType, int flag,
    bool editable, bool userEditable)
: ColumnBase (title, displayType, flag), mEditable (editable), mUserEditable (userEditable)
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

    mColumns.push_back (RefIdColumn ("ID", ColumnBase::Display_String,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, false, false));
    baseColumns.mId = &mColumns.back();
    mColumns.push_back (RefIdColumn ("*", ColumnBase::Display_RecordState,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, false, false));
    baseColumns.mModified = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Type", ColumnBase::Display_Integer,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, false, false));
    baseColumns.mType = &mColumns.back();

    ModelColumns modelColumns (baseColumns);

    mColumns.push_back (RefIdColumn ("Model", ColumnBase::Display_String));
    modelColumns.mModel = &mColumns.back();

    NameColumns nameColumns (modelColumns);

    mColumns.push_back (RefIdColumn ("Name", ColumnBase::Display_String));
    nameColumns.mName = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Script", ColumnBase::Display_String));
    nameColumns.mScript = &mColumns.back();

    InventoryColumns inventoryColumns (nameColumns);

    mColumns.push_back (RefIdColumn ("Icon", ColumnBase::Display_String));
    inventoryColumns.mIcon = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Weight", ColumnBase::Display_Float));
    inventoryColumns.mWeight = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Value", ColumnBase::Display_Integer));
    inventoryColumns.mValue = &mColumns.back();

    EnchantableColumns enchantableColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn ("Enchantment", ColumnBase::Display_String));
    enchantableColumns.mEnchantment = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Enchantment Points", ColumnBase::Display_Integer));
    enchantableColumns.mEnchantmentPoints = &mColumns.back();

    ToolColumns toolsColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn ("Quality", ColumnBase::Display_Float));
    toolsColumns.mQuality = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Uses", ColumnBase::Display_Integer));
    toolsColumns.mUses = &mColumns.back();

    ActorColumns actorsColumns (nameColumns);

    mColumns.push_back (RefIdColumn ("AI", ColumnBase::Display_Boolean));
    actorsColumns.mHasAi = &mColumns.back();
    mColumns.push_back (RefIdColumn ("AI Hello", ColumnBase::Display_Integer));
    actorsColumns.mHello = &mColumns.back();
    mColumns.push_back (RefIdColumn ("AI Flee", ColumnBase::Display_Integer));
    actorsColumns.mFlee = &mColumns.back();
    mColumns.push_back (RefIdColumn ("AI Fight", ColumnBase::Display_Integer));
    actorsColumns.mFight = &mColumns.back();
    mColumns.push_back (RefIdColumn ("AI Alarm", ColumnBase::Display_Integer));
    actorsColumns.mAlarm = &mColumns.back();

    static const struct
    {
        const char *mName;
        unsigned int mFlag;
    } sServiceTable[] =
    {
        { "Buys Weapons", ESM::NPC::Weapon},
        { "Buys Armor", ESM::NPC::Armor},
        { "Buys Clothing", ESM::NPC::Clothing},
        { "Buys Books", ESM::NPC::Books},
        { "Buys Ingredients", ESM::NPC::Ingredients},
        { "Buys Lockpicks", ESM::NPC::Picks},
        { "Buys Probes", ESM::NPC::Probes},
        { "Buys Lights", ESM::NPC::Lights},
        { "Buys Apparati", ESM::NPC::Apparatus},
        { "Buys Repair Items", ESM::NPC::RepairItem},
        { "Buys Misc Items", ESM::NPC::Misc},
        { "Buys Potions", ESM::NPC::Potions},
        { "Buys Magic Items", ESM::NPC::MagicItems},
        { "Sells Spells", ESM::NPC::Spells},
        { "Trainer", ESM::NPC::Training},
        { "Spellmaking", ESM::NPC::Spellmaking},
        { "Enchanting Service", ESM::NPC::Enchanting},
        { "Repair Serivce", ESM::NPC::Repair},
        { 0, 0 }
    };

    for (int i=0; sServiceTable[i].mName; ++i)
    {
        mColumns.push_back (RefIdColumn (sServiceTable[i].mName, ColumnBase::Display_Boolean));
        actorsColumns.mServices.insert (std::make_pair (&mColumns.back(), sServiceTable[i].mFlag));
    }

    mColumns.push_back (RefIdColumn ("Auto Calc", ColumnBase::Display_Boolean));
    const RefIdColumn *autoCalc = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Apparatus Type", ColumnBase::Display_ApparatusType));
    const RefIdColumn *apparatusType = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Armor Type", ColumnBase::Display_ArmorType));
    const RefIdColumn *armorType = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Health", ColumnBase::Display_Integer));
    const RefIdColumn *health = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Armor Value", ColumnBase::Display_Integer));
    const RefIdColumn *armor = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Scroll", ColumnBase::Display_Boolean));
    const RefIdColumn *scroll = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Attribute", ColumnBase::Display_Attribute));
    const RefIdColumn *attribute = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Clothing Type", ColumnBase::Display_ClothingType));
    const RefIdColumn *clothingType = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Weight Capacity", ColumnBase::Display_Float));
    const RefIdColumn *weightCapacity = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Organic Container", ColumnBase::Display_Boolean));
    const RefIdColumn *organic = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Respawn", ColumnBase::Display_Boolean));
    const RefIdColumn *respawn = &mColumns.back();

    CreatureColumns creatureColumns (actorsColumns);

    mColumns.push_back (RefIdColumn ("Creature Type", ColumnBase::Display_CreatureType));
    creatureColumns.mType = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Soul Points", ColumnBase::Display_Integer));
    creatureColumns.mSoul = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Scale", ColumnBase::Display_Float));
    creatureColumns.mScale = &mColumns.back();
    mColumns.push_back (RefIdColumn ("Original Creature", ColumnBase::Display_String));
    creatureColumns.mOriginal = &mColumns.back();

    static const struct
    {
        const char *mName;
        unsigned int mFlag;
    } sCreatureFlagTable[] =
    {
        { "Biped", ESM::Creature::Biped },
        { "Has Weapon", ESM::Creature::Weapon },
        { "No Movement", ESM::Creature::None },
        { "Swims", ESM::Creature::Swims },
        { "Flies", ESM::Creature::Flies },
        { "Walks", ESM::Creature::Walks },
        { "Essential", ESM::Creature::Essential },
        { "Skeleton Blood", ESM::Creature::Skeleton },
        { "Metal Blood", ESM::Creature::Metal },
        { 0, 0 }
    };

    // for re-use in NPC records
    const RefIdColumn *essential = 0;
    const RefIdColumn *skeletonBlood = 0;
    const RefIdColumn *metalBlood = 0;

    for (int i=0; sCreatureFlagTable[i].mName; ++i)
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

    mColumns.push_back (RefIdColumn ("Open Sound", ColumnBase::Display_String));
    const RefIdColumn *openSound = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Close Sound", ColumnBase::Display_String));
    const RefIdColumn *closeSound = &mColumns.back();

    LightColumns lightColumns (inventoryColumns);

    mColumns.push_back (RefIdColumn ("Duration", ColumnBase::Display_Integer));
    lightColumns.mTime = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Radius", ColumnBase::Display_Integer));
    lightColumns.mRadius = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Colour", ColumnBase::Display_Integer));
    lightColumns.mColor = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Sound", ColumnBase::Display_String));
    lightColumns.mSound = &mColumns.back();

    static const struct
    {
        const char *mName;
        unsigned int mFlag;
    } sLightFlagTable[] =
    {
        { "Dynamic", ESM::Light::Dynamic },
        { "Portable", ESM::Light::Carry },
        { "Negative Light", ESM::Light::Negative },
        { "Flickering", ESM::Light::Flicker },
        { "Slow Flickering", ESM::Light::Flicker },
        { "Pulsing", ESM::Light::Pulse },
        { "Slow Pulsing", ESM::Light::PulseSlow },
        { "Fire", ESM::Light::Fire },
        { "Off by default", ESM::Light::OffDefault },
        { 0, 0 }
    };

    for (int i=0; sLightFlagTable[i].mName; ++i)
    {
        mColumns.push_back (RefIdColumn (sLightFlagTable[i].mName, ColumnBase::Display_Boolean));
        lightColumns.mFlags.insert (std::make_pair (&mColumns.back(), sLightFlagTable[i].mFlag));
    }

    mColumns.push_back (RefIdColumn ("Key", ColumnBase::Display_Boolean));
    const RefIdColumn *key = &mColumns.back();

    NpcColumns npcColumns (actorsColumns);

    mColumns.push_back (RefIdColumn ("Race", ColumnBase::Display_String));
    npcColumns.mRace = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Class", ColumnBase::Display_String));
    npcColumns.mClass = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Faction", ColumnBase::Display_String));
    npcColumns.mFaction = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Hair", ColumnBase::Display_String));
    npcColumns.mHair = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Head", ColumnBase::Display_String));
    npcColumns.mHead = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Female", ColumnBase::Display_Boolean));
    npcColumns.mFlags.insert (std::make_pair (&mColumns.back(), ESM::NPC::Female));

    npcColumns.mFlags.insert (std::make_pair (essential, ESM::NPC::Essential));

    npcColumns.mFlags.insert (std::make_pair (respawn, ESM::NPC::Respawn));

    npcColumns.mFlags.insert (std::make_pair (autoCalc, ESM::NPC::Autocalc));

    npcColumns.mFlags.insert (std::make_pair (skeletonBlood, ESM::NPC::Skeleton));

    npcColumns.mFlags.insert (std::make_pair (metalBlood, ESM::NPC::Metal));

    WeaponColumns weaponColumns (enchantableColumns);

    mColumns.push_back (RefIdColumn ("Weapon Type", ColumnBase::Display_WeaponType));
    weaponColumns.mType = &mColumns.back();

    weaponColumns.mHealth = health;

    mColumns.push_back (RefIdColumn ("Weapon Speed", ColumnBase::Display_Float));
    weaponColumns.mSpeed = &mColumns.back();

    mColumns.push_back (RefIdColumn ("Weapon Reach", ColumnBase::Display_Float));
    weaponColumns.mReach = &mColumns.back();

    for (int i=0; i<2; ++i)
    {
        std::string suffix = i==0 ? "Min " : "Max ";

        mColumns.push_back (RefIdColumn ("Chop" + suffix, ColumnBase::Display_Integer));
        weaponColumns.mChop[i] = &mColumns.back();

        mColumns.push_back (RefIdColumn ("Slash" + suffix, ColumnBase::Display_Integer));
        weaponColumns.mSlash[i] = &mColumns.back();

        mColumns.push_back (RefIdColumn ("Thrust" + suffix, ColumnBase::Display_Integer));
        weaponColumns.mThrust[i] = &mColumns.back();
    }

    static const struct
    {
        const char *mName;
        unsigned int mFlag;
    } sWeaponFlagTable[] =
    {
        { "Magical", ESM::Weapon::Magical },
        { "Silver", ESM::Weapon::Silver },
        { 0, 0 }
    };

    for (int i=0; sWeaponFlagTable[i].mName; ++i)
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
    mData.appendRecord (type, id);
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

void CSMWorld::RefIdCollection::appendRecord (const RecordBase& record,
    UniversalId::Type type)
{
    std::string id = findAdaptor (type).getId (record);

    int index = mData.getAppendIndex (type);

    mData.appendRecord (type, id);

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
            mData.appendRecord (type, id);

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

int CSMWorld::RefIdCollection::getAppendIndex (UniversalId::Type type) const
{
    return mData.getAppendIndex (type);
}
