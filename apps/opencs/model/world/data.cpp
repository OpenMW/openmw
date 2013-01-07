
#include "data.hpp"

#include <stdexcept>

#include <QAbstractTableModel>

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

    addModel (new IdTable (&mGlobals), UniversalId::Type_Globals, UniversalId::Type_Global);
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