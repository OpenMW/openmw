#include "tablemimedata.hpp"
#include "universalid.hpp"
#include <string>

CSMWorld::TableMimeData::TableMimeData (UniversalId id)
{
    mUniversalId.push_back(id);
    mObjectsFormats << QString::fromStdString("tabledata/" + id.getTypeName());
}

CSMWorld::TableMimeData::TableMimeData (std::vector< CSMWorld::UniversalId >& id)
{
    mUniversalId = id;
    for (std::vector<UniversalId>::iterator it(mUniversalId.begin()); it != mUniversalId.end(); ++it)
    {
        mObjectsFormats << QString::fromStdString("tabledata/" + it->getTypeName());
    }
}

QStringList CSMWorld::TableMimeData::formats() const
{
    return mObjectsFormats;
}

CSMWorld::TableMimeData::~TableMimeData()
{
}

std::string CSMWorld::TableMimeData::getIcon() const
{
    if (mUniversalId.empty())
    {
        throw("TableMimeData holds no UniversalId");
    }

    std::string tmpIcon;
    bool firstIteration = true;
    for (unsigned i = 0; i < mUniversalId.size(); ++i)
    {
        if (firstIteration)
        {
            firstIteration = false;
            tmpIcon = mUniversalId[i].getIcon();
            continue;
        }

        if (tmpIcon != mUniversalId[i].getIcon())
        {
            return ""; //should return multiple types icon, but at the moment we don't have one
        }

        tmpIcon = mUniversalId[i].getIcon();
    }
    return mUniversalId.begin()->getIcon(); //All objects are of the same type;
}

std::vector< CSMWorld::UniversalId > CSMWorld::TableMimeData::getData() const
{
    return mUniversalId;
}