
#include "data.hpp"

#include <stdexcept>

#include <QAbstractTableModel>

#include <components/esm/loadglob.hpp>

#include "idtable.hpp"
#include "columns.hpp"

CSMWorld::Data::Data()
{
    mGlobals.addColumn (new FloatValueColumn<ESM::Global>);

    mModels.insert (std::make_pair (
        UniversalId (UniversalId::Type_Globals),
        new IdTable (&mGlobals)
        ));
}

CSMWorld::Data::~Data()
{
    for (std::map<UniversalId, QAbstractTableModel *>::iterator iter (mModels.begin()); iter!=mModels.end(); ++iter)
        delete iter->second;
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
    std::map<UniversalId, QAbstractTableModel *>::iterator iter = mModels.find (id);

    if (iter==mModels.end())
        throw std::logic_error ("No table model available for " + id.toString());

    return iter->second;
}