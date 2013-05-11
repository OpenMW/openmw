
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
    mColumns.push_back (RefIdColumn ("*", ColumnBase::Display_Integer,
        ColumnBase::Flag_Table | ColumnBase::Flag_Dialogue, false, false));
    baseColumns.mModified = &mColumns.back();
//    mColumns.push_back (RefIdColumn ("Name", ColumnBase::Display_String));



    mAdapters.insert (std::make_pair (UniversalId::Type_Activator,
        new BaseRefIdAdapter<ESM::Activator> (UniversalId::Type_Activator, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Potion,
        new BaseRefIdAdapter<ESM::Potion> (UniversalId::Type_Potion, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Apparatus,
        new BaseRefIdAdapter<ESM::Apparatus> (UniversalId::Type_Apparatus, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Armor,
        new BaseRefIdAdapter<ESM::Armor> (UniversalId::Type_Armor, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Book,
        new BaseRefIdAdapter<ESM::Book> (UniversalId::Type_Book, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Clothing,
        new BaseRefIdAdapter<ESM::Clothing> (UniversalId::Type_Clothing, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Container,
        new BaseRefIdAdapter<ESM::Container> (UniversalId::Type_Container, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Creature,
        new BaseRefIdAdapter<ESM::Creature> (UniversalId::Type_Creature, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Door,
        new BaseRefIdAdapter<ESM::Door> (UniversalId::Type_Door, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Ingredient,
        new BaseRefIdAdapter<ESM::Ingredient> (UniversalId::Type_Ingredient, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_CreatureLevelledList,
        new BaseRefIdAdapter<ESM::CreatureLevList> (
        UniversalId::Type_CreatureLevelledList, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_ItemLevelledList,
        new BaseRefIdAdapter<ESM::ItemLevList> (UniversalId::Type_ItemLevelledList, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Light,
        new BaseRefIdAdapter<ESM::Light> (UniversalId::Type_Light, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Lockpick,
        new BaseRefIdAdapter<ESM::Lockpick> (UniversalId::Type_Lockpick, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Miscellaneous,
        new BaseRefIdAdapter<ESM::Miscellaneous> (UniversalId::Type_Miscellaneous, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Npc,
        new BaseRefIdAdapter<ESM::NPC> (UniversalId::Type_Npc, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Probe,
        new BaseRefIdAdapter<ESM::Probe> (UniversalId::Type_Probe, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Repair,
        new BaseRefIdAdapter<ESM::Repair> (UniversalId::Type_Repair, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Static,
        new BaseRefIdAdapter<ESM::Static> (UniversalId::Type_Static, baseColumns)));
    mAdapters.insert (std::make_pair (UniversalId::Type_Weapon,
        new BaseRefIdAdapter<ESM::Weapon> (UniversalId::Type_Weapon, baseColumns)));
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