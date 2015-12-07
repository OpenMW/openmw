#include "regionmap.hpp"

#include <cmath>
#include <algorithm>

#include <QBrush>

#include <components/misc/stringops.hpp>

#include "data.hpp"
#include "universalid.hpp"

CSMWorld::RegionMap::CellDescription::CellDescription() : mDeleted (false) {}

CSMWorld::RegionMap::CellDescription::CellDescription (const Record<Cell>& cell)
{
    const Cell& cell2 = cell.get();

    if (!cell2.isExterior())
        throw std::logic_error ("Interior cell in region map");

    mDeleted = cell.isDeleted();

    mRegion = cell2.mRegion;
    mName = cell2.mName;
}

CSMWorld::CellCoordinates CSMWorld::RegionMap::getIndex (const QModelIndex& index) const
{
    return mMin.move (index.column(), mMax.getY()-mMin.getY() - index.row()-1);
}

QModelIndex CSMWorld::RegionMap::getIndex (const CellCoordinates& index) const
{
    // I hate you, Qt API naming scheme!
    return QAbstractTableModel::index (mMax.getY()-mMin.getY() - (index.getY()-mMin.getY())-1,
        index.getX()-mMin.getX());
}

CSMWorld::CellCoordinates CSMWorld::RegionMap::getIndex (const Cell& cell) const
{
    std::istringstream stream (cell.mId);

    char ignore;
    int x = 0;
    int y = 0;
    stream >> ignore >> x >> y;

    return CellCoordinates (x, y);
}

void CSMWorld::RegionMap::buildRegions()
{
    const IdCollection<ESM::Region>& regions = mData.getRegions();

    int size = regions.getSize();

    for (int i=0; i<size; ++i)
    {
        const Record<ESM::Region>& region = regions.getRecord (i);

        if (!region.isDeleted())
            mColours.insert (std::make_pair (Misc::StringUtils::lowerCase (region.get().mId),
                region.get().mMapColor));
    }
}

void CSMWorld::RegionMap::buildMap()
{
    const IdCollection<Cell>& cells = mData.getCells();

    int size = cells.getSize();

    for (int i=0; i<size; ++i)
    {
        const Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.isExterior())
        {
            CellDescription description (cell);

            CellCoordinates index = getIndex (cell2);

            mMap.insert (std::make_pair (index, description));
        }
    }

    std::pair<CellCoordinates, CellCoordinates> mapSize = getSize();

    mMin = mapSize.first;
    mMax = mapSize.second;
}

void CSMWorld::RegionMap::addCell (const CellCoordinates& index, const CellDescription& description)
{
    std::map<CellCoordinates, CellDescription>::iterator cell = mMap.find (index);

    if (cell!=mMap.end())
    {
        cell->second = description;
    }
    else
    {
        updateSize();

        mMap.insert (std::make_pair (index, description));
    }

    QModelIndex index2 = getIndex (index);

    dataChanged (index2, index2);
}

void CSMWorld::RegionMap::addCells (int start, int end)
{
    const IdCollection<Cell>& cells = mData.getCells();

    for (int i=start; i<=end; ++i)
    {
        const Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.isExterior())
        {
            CellCoordinates index = getIndex (cell2);

            CellDescription description (cell);

            addCell (index, description);
        }
    }
}

void CSMWorld::RegionMap::removeCell (const CellCoordinates& index)
{
    std::map<CellCoordinates, CellDescription>::iterator cell = mMap.find (index);

    if (cell!=mMap.end())
    {
        mMap.erase (cell);

        QModelIndex index2 = getIndex (index);

        dataChanged (index2, index2);

        updateSize();
    }
}

void CSMWorld::RegionMap::addRegion (const std::string& region, unsigned int colour)
{
    mColours[Misc::StringUtils::lowerCase (region)] = colour;
}

void CSMWorld::RegionMap::removeRegion (const std::string& region)
{
    std::map<std::string, unsigned int>::iterator iter (
        mColours.find (Misc::StringUtils::lowerCase (region)));

    if (iter!=mColours.end())
        mColours.erase (iter);
}

void CSMWorld::RegionMap::updateRegions (const std::vector<std::string>& regions)
{
    std::vector<std::string> regions2 (regions);

    std::for_each (regions2.begin(), regions2.end(), Misc::StringUtils::lowerCaseInPlace);
    std::sort (regions2.begin(), regions2.end());

    for (std::map<CellCoordinates, CellDescription>::const_iterator iter (mMap.begin());
         iter!=mMap.end(); ++iter)
    {
        if (!iter->second.mRegion.empty() &&
            std::find (regions2.begin(), regions2.end(),
            Misc::StringUtils::lowerCase (iter->second.mRegion))!=regions2.end())
        {
            QModelIndex index = getIndex (iter->first);

            dataChanged (index, index);
        }
    }
}

void CSMWorld::RegionMap::updateSize()
{
    std::pair<CellCoordinates, CellCoordinates> size = getSize();

    if (int diff = size.first.getX() - mMin.getX())
    {
        beginInsertColumns (QModelIndex(), 0, std::abs (diff)-1);
        mMin = CellCoordinates (size.first.getX(), mMin.getY());
        endInsertColumns();
    }

    if (int diff = size.first.getY() - mMin.getY())
    {
        beginInsertRows (QModelIndex(), 0, std::abs (diff)-1);
        mMin = CellCoordinates (mMin.getX(), size.first.getY());
        endInsertRows();
    }

    if (int diff = size.second.getX() - mMax.getX())
    {
        int columns = columnCount();

        if (diff>0)
            beginInsertColumns (QModelIndex(), columns, columns+diff-1);
        else
            beginRemoveColumns (QModelIndex(), columns+diff, columns-1);

        mMax = CellCoordinates (size.second.getX(), mMax.getY());
        endInsertColumns();
    }

    if (int diff = size.second.getY() - mMax.getY())
    {
        int rows = rowCount();

        if (diff>0)
            beginInsertRows (QModelIndex(), rows, rows+diff-1);
        else
            beginRemoveRows (QModelIndex(), rows+diff, rows-1);

        mMax = CellCoordinates (mMax.getX(), size.second.getY());
        endInsertRows();
    }
}

std::pair<CSMWorld::CellCoordinates, CSMWorld::CellCoordinates> CSMWorld::RegionMap::getSize() const
{
    const IdCollection<Cell>& cells = mData.getCells();

    int size = cells.getSize();

    CellCoordinates min (0, 0);
    CellCoordinates max (0, 0);

    for (int i=0; i<size; ++i)
    {
        const Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.isExterior())
        {
            CellCoordinates index = getIndex (cell2);

            if (min==max)
            {
                min = index;
                max = min.move (1, 1);
            }
            else
            {
                if (index.getX()<min.getX())
                    min = CellCoordinates (index.getX(), min.getY());
                else if (index.getX()>=max.getX())
                    max = CellCoordinates (index.getX()+1, max.getY());

                if (index.getY()<min.getY())
                    min = CellCoordinates (min.getX(), index.getY());
                else if (index.getY()>=max.getY())
                    max = CellCoordinates (max.getX(), index.getY() + 1);
            }
        }
    }

    return std::make_pair (min, max);
}

CSMWorld::RegionMap::RegionMap (Data& data) : mData (data)
{
    buildRegions();
    buildMap();

    QAbstractItemModel *regions = data.getTableModel (UniversalId (UniversalId::Type_Regions));

    connect (regions, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (regionsAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (regions, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (regionsInserted (const QModelIndex&, int, int)));
    connect (regions, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (regionsChanged (const QModelIndex&, const QModelIndex&)));

    QAbstractItemModel *cells = data.getTableModel (UniversalId (UniversalId::Type_Cells));

    connect (cells, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellsAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (cellsInserted (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellsChanged (const QModelIndex&, const QModelIndex&)));
}

int CSMWorld::RegionMap::rowCount (const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mMax.getY()-mMin.getY();
}

int CSMWorld::RegionMap::columnCount (const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mMax.getX()-mMin.getX();
}

QVariant CSMWorld::RegionMap::data (const QModelIndex& index, int role) const
{
    if (role==Qt::SizeHintRole)
        return QSize (16, 16);

    if (role==Qt::BackgroundRole)
    {
        /// \todo GUI class in non-GUI code. Needs to be addressed eventually.

        std::map<CellCoordinates, CellDescription>::const_iterator cell =
            mMap.find (getIndex (index));

        if (cell!=mMap.end())
        {
            if (cell->second.mDeleted)
                return QBrush (Qt::red, Qt::DiagCrossPattern);

            std::map<std::string, unsigned int>::const_iterator iter =
                mColours.find (Misc::StringUtils::lowerCase (cell->second.mRegion));

            if (iter!=mColours.end())
                return QBrush (QColor (iter->second & 0xff, 
                                       (iter->second >> 8) & 0xff, 
                                       (iter->second >> 16) & 0xff));

            if (cell->second.mRegion.empty())
                return QBrush (Qt::Dense6Pattern); // no region

            return QBrush (Qt::red, Qt::Dense6Pattern); // invalid region
        }

        return QBrush (Qt::DiagCrossPattern);
    }

    if (role==Qt::ToolTipRole)
    {
        CellCoordinates cellIndex = getIndex (index);

        std::ostringstream stream;

        stream << cellIndex;

        std::map<CellCoordinates, CellDescription>::const_iterator cell =
            mMap.find (cellIndex);

        if (cell!=mMap.end())
        {
            if (!cell->second.mName.empty())
                stream << " " << cell->second.mName;

            if (cell->second.mDeleted)
                stream << " (deleted)";

            if (!cell->second.mRegion.empty())
            {
                stream << "<br>";

                std::map<std::string, unsigned int>::const_iterator iter =
                    mColours.find (Misc::StringUtils::lowerCase (cell->second.mRegion));

                if (iter!=mColours.end())
                    stream << cell->second.mRegion;
                else
                    stream << "<font color=red>" << cell->second.mRegion << "</font>";
            }
        }
        else
            stream << " (no cell)";

        return QString::fromUtf8 (stream.str().c_str());
    }

    if (role==Role_Region)
    {
        CellCoordinates cellIndex = getIndex (index);

        std::map<CellCoordinates, CellDescription>::const_iterator cell =
            mMap.find (cellIndex);

        if (cell!=mMap.end() && !cell->second.mRegion.empty())
            return QString::fromUtf8 (Misc::StringUtils::lowerCase (cell->second.mRegion).c_str());
    }

    if (role==Role_CellId)
    {
        CellCoordinates cellIndex = getIndex (index);

        std::ostringstream stream;
        stream << "#" << cellIndex.getX() << " " << cellIndex.getY();

        return QString::fromUtf8 (stream.str().c_str());
    }

    return QVariant();
}

Qt::ItemFlags CSMWorld::RegionMap::flags (const QModelIndex& index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void CSMWorld::RegionMap::regionsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{
    std::vector<std::string> update;

    const IdCollection<ESM::Region>& regions = mData.getRegions();

    for (int i=start; i<=end; ++i)
    {
        const Record<ESM::Region>& region = regions.getRecord (i);

        update.push_back (region.get().mId);

        removeRegion (region.get().mId);
    }

    updateRegions (update);
}

void CSMWorld::RegionMap::regionsInserted (const QModelIndex& parent, int start, int end)
{
    std::vector<std::string> update;

    const IdCollection<ESM::Region>& regions = mData.getRegions();

    for (int i=start; i<=end; ++i)
    {
        const Record<ESM::Region>& region = regions.getRecord (i);

        if (!region.isDeleted())
        {
            update.push_back (region.get().mId);

            addRegion (region.get().mId, region.get().mMapColor);
        }
    }

    updateRegions (update);
}

void CSMWorld::RegionMap::regionsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // Note: At this point an additional check could be inserted to see if there is any change to the
    // columns we are interested in. If not we can exit the function here and avoid all updating.

    std::vector<std::string> update;

    const IdCollection<ESM::Region>& regions = mData.getRegions();

    for (int i=topLeft.row(); i<=bottomRight.column(); ++i)
    {
        const Record<ESM::Region>& region = regions.getRecord (i);

        update.push_back (region.get().mId);

        if (!region.isDeleted())
            addRegion (region.get().mId, region.get().mMapColor);
        else
            removeRegion (region.get().mId);
    }

    updateRegions (update);
}

void CSMWorld::RegionMap::cellsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{
    const IdCollection<Cell>& cells = mData.getCells();

    for (int i=start; i<=end; ++i)
    {
        const Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.isExterior())
            removeCell (getIndex (cell2));
    }
}

void CSMWorld::RegionMap::cellsInserted (const QModelIndex& parent, int start, int end)
{
    addCells (start, end);
}

void CSMWorld::RegionMap::cellsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // Note: At this point an additional check could be inserted to see if there is any change to the
    // columns we are interested in. If not we can exit the function here and avoid all updating.

    addCells (topLeft.row(), bottomRight.row());
}
