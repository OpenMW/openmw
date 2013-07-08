
#include "regionmap.hpp"

#include <QBrush>

#include "data.hpp"
#include "universalid.hpp"

std::pair<int, int> CSMWorld::RegionMap::getIndex (const QModelIndex& index) const
{
    return std::make_pair (index.column()+mMin.first, index.row()+mMin.second);
}

void CSMWorld::RegionMap::buildRegions (Data& data)
{
    const IdCollection<ESM::Region>& regions = data.getRegions();

    int size = regions.getSize();

    for (int i=0; i<size; ++i)
    {
        const Record<ESM::Region>& region = regions.getRecord (i);

        if (!region.isDeleted())
            mColours.insert (std::make_pair (region.get().mId, region.get().mMapColor));
    }
}

void CSMWorld::RegionMap::buildMap (Data& data)
{
    const IdCollection<Cell>& cells = data.getCells();

    int size = cells.getSize();

    mMin = mMax = std::make_pair (0, 0);

    for (int i=0; i<size; ++i)
    {
        const Record<Cell>& cell = cells.getRecord (i);

        if (!cell.isDeleted())
        {
            const Cell& cell2 = cell.get();

            if (cell2.isExterior())
            {
                std::pair<int, int> index (cell2.mData.mX, cell2.mData.mY);

                if (mMap.empty())
                {
                    mMin = index;
                    mMax = std::make_pair (mMin.first+1, mMin.second+1);
                }
                else
                {
                    if (index.first<mMin.first)
                        mMin.first = index.first;
                    else if (index.first>=mMax.first)
                        mMax.first = index.first + 1;

                    if (index.second<mMin.second)
                        mMin.second = index.second;
                    else if (index.second>=mMax.second)
                        mMax.second = index.second + 1;
                }

                mMap.insert (std::make_pair (index, cell2.mRegion));
            }
        }
    }
}

CSMWorld::RegionMap::RegionMap (Data& data)
{
    buildRegions (data);
    buildMap (data);

    QAbstractItemModel *regions = data.getTableModel (UniversalId (UniversalId::Type_Regions));

    connect (regions, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (regionsAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (regions, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (regionsInserted (const QModelIndex&, int, int)));
    connect (regions, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (regionsChanged (const QModelIndex&, const QModelIndex&)));

    QAbstractItemModel *cells = data.getTableModel (UniversalId (UniversalId::Type_Cells));

    connect (cells, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (regionsAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (regionsInserted (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (regionsChanged (const QModelIndex&, const QModelIndex&)));
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

            if (cell->second.empty())
                return QBrush (Qt::Dense6Pattern); // no region

            return QBrush (Qt::red, Qt::Dense6Pattern);
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

void CSMWorld::RegionMap::regionsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{


}

void CSMWorld::RegionMap::regionsInserted (const QModelIndex& parent, int start, int end)
{


}

void CSMWorld::RegionMap::regionsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{


}

void CSMWorld::RegionMap::cellsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{


}

void CSMWorld::RegionMap::cellsInserted (const QModelIndex& parent, int start, int end)
{


}

void CSMWorld::RegionMap::cellsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
;
}