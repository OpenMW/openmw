
#include "data.hpp"

#include <stdexcept>

#include <QAbstractTableModel>

#include <components/esm/esmreader.hpp>
#include <components/esm/loadglob.hpp>

#include "idtable.hpp"
#include "columns.hpp"

void CSMWorld::Data::addModel (QAbstractTableModel *model, UniversalId::Type type1,
    UniversalId::Type type2)
{
    mModels.push_back (model);
    mModelIndex.insert (std::make_pair (type1, model));

    if (type2!=UniversalId::Type_None)
        mModelIndex.insert (std::make_pair (type2, model));
}

CSMWorld::Data::Data()
{
    mGlobals.addColumn (new StringIdColumn<ESM::Global>);
    mGlobals.addColumn (new RecordStateColumn<ESM::Global>);
    mGlobals.addColumn (new FixedRecordTypeColumn<ESM::Global> (UniversalId::Type_Global));
    mGlobals.addColumn (new FloatValueColumn<ESM::Global>);

    mGmsts.addColumn (new StringIdColumn<ESM::GameSetting>);
    mGmsts.addColumn (new RecordStateColumn<ESM::GameSetting>);
    mGmsts.addColumn (new FixedRecordTypeColumn<ESM::GameSetting> (UniversalId::Type_Gmst));
    mGmsts.addColumn (new VarTypeColumn<ESM::GameSetting>);
    mGmsts.addColumn (new VarValueColumn<ESM::GameSetting>);

    addModel (new IdTable (&mGlobals), UniversalId::Type_Globals, UniversalId::Type_Global);
    addModel (new IdTable (&mGmsts), UniversalId::Type_Gmsts, UniversalId::Type_Gmst);
}

CSMWorld::Data::~Data()
{
    for (std::vector<QAbstractTableModel *>::iterator iter (mModels.begin()); iter!=mModels.end(); ++iter)
        delete *iter;
}

const CSMWorld::IdCollection<ESM::Global>& CSMWorld::Data::getGlobals() const
{
    return mGlobals;
}

CSMWorld::IdCollection<ESM::Global>& CSMWorld::Data::getGlobals()
{
    return mGlobals;
}

QAbstractTableModel *CSMWorld::Data::getTableModel (const UniversalId& id)
{
    std::map<UniversalId::Type, QAbstractTableModel *>::iterator iter = mModelIndex.find (id.getType());

    if (iter==mModelIndex.end())
        throw std::logic_error ("No table model available for " + id.toString());

    return iter->second;
}

void CSMWorld::Data::merge()
{
    mGlobals.merge();
}

void CSMWorld::Data::loadFile (const boost::filesystem::path& path, bool base)
{
    ESM::ESMReader reader;
    /// \todo set encoder
    reader.open (path.string());

    // Note: We do not need to send update signals here, because at this point the model is not connected
    // to any view.
    while (reader.hasMoreRecs())
    {
        ESM::NAME n = reader.getRecName();
        reader.getRecHeader();

        switch (n.val)
        {
            case ESM::REC_GLOB: mGlobals.load (reader, base); break;
            case ESM::REC_GMST: mGmsts.load (reader, base); break;


            default:

                /// \todo throw an exception instead, once all records are implemented
                reader.skipRecord();
        }
    }
}