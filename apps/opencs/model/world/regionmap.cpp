
#include "regionmap.hpp"

#include <QBrush>

std::pair<int, int> CSMWorld::RegionMap::getIndex (const QModelIndex& index) const
{
    return std::make_pair (index.column()+mMin.first, index.row()+mMin.second);
}

CSMWorld::RegionMap::RegionMap()
{
    // setting up some placeholder regions
    mMap.insert (std::make_pair (std::make_pair (0, 0), "a"));
    mMap.insert (std::make_pair (std::make_pair (1, 1), "b"));
    mMap.insert (std::make_pair (std::make_pair (1, 0), "a"));
    mMin = std::make_pair (0, 0);
    mMax = std::make_pair (2, 2);
    mColours.insert (std::make_pair ("a", 0xff0000ff));
    mColours.insert (std::make_pair ("b", 0x00ff00ff));
}

int CSMWorld::RegionMap::rowCount (const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mMax.second-mMin.second;
}

int CSMWorld::RegionMap::columnCount (const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mMax.first-mMin.first;
}

QVariant CSMWorld::RegionMap::data (const QModelIndex& index, int role) const
{
    if (role==Qt::SizeHintRole)
        return QSize (16, 16);

    if (role==Qt::BackgroundRole)
    {
        /// \todo GUI class in non-GUI code. Needs to be addressed eventually.

        std::map<std::pair<int, int>, std::string>::const_iterator cell =
            mMap.find (getIndex (index));

        if (cell!=mMap.end())
        {
            std::map<std::string, unsigned int>::const_iterator iter = mColours.find (cell->second);

            if (iter!=mColours.end())
                return QBrush (
                    QColor (iter->second>>24, (iter->second>>16) & 255, (iter->second>>8) & 255,
                    iter->second & 255));
        }

        return QBrush (Qt::DiagCrossPattern);
    }

    return QVariant();
}

Qt::ItemFlags CSMWorld::RegionMap::flags (const QModelIndex& index) const
{
    if (mMap.find (getIndex (index))!=mMap.end())
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    return 0;
}