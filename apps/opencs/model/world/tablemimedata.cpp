#include "tablemimedata.hpp"
#include "universalid.hpp"

CSMWorld::TableMimeData::TableMimeData (CSMWorld::UniversalId& UniversalId) :
mUniversalId(UniversalId)
{
    mSupportedFormats << UniversalId.toString().c_str();
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
