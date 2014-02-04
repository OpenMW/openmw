#include "tablemimedata.hpp"
#include "universalid.hpp"

CSMWorld::TableMimeData::TableMimeData (UniversalId id) :
mUniversalId(id)
{
    mSupportedFormats << QString::fromStdString("application/Type_" + id.getTypeName());
}

QStringList CSMWorld::TableMimeData::formats() const
{
    return QMimeData::formats();
}

CSMWorld::TableMimeData::~TableMimeData()
{
}

CSMWorld::UniversalId& CSMWorld::TableMimeData::getId()
{
    return mUniversalId;
}