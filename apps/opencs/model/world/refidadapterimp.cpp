
#include "refidadapterimp.hpp"

#include <QVariant>

#include <components/esm/loadstat.hpp>

#include "record.hpp"
#include "refiddata.hpp"
#include "universalid.hpp"

CSMWorld::StaticRefIdAdapter::StaticRefIdAdapter (const RefIdColumn *id, const RefIdColumn *modified)
: mId (id), mModified (modified)
{}

QVariant CSMWorld::StaticRefIdAdapter::getData (const RefIdColumn *column, const RefIdData& data,
    int index) const
{
    const Record<ESM::Static>& record = static_cast<const Record<ESM::Static>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Static)));

    if (column==mId)
        return QString::fromUtf8 (record.get().mId.c_str());

    if (column==mModified)
    {
        if (record.mState==Record<ESM::Static>::State_Erased)
            return static_cast<int> (Record<ESM::Static>::State_Deleted);

        return static_cast<int> (record.mState);
    }

    return QVariant();
}

void CSMWorld::StaticRefIdAdapter::setData (const RefIdColumn *column, RefIdData& data, int index,
    const QVariant& value) const
{
    Record<ESM::Static>& record = static_cast<Record<ESM::Static>&> (
        data.getRecord (RefIdData::LocalIndex (index, UniversalId::Type_Static)));

    if (column==mModified)
        record.mState = static_cast<RecordBase::State> (value.toInt());
}

std::string CSMWorld::StaticRefIdAdapter::getId (const RecordBase& record) const
{
    return dynamic_cast<const Record<ESM::Static>&> (record).get().mId;
}