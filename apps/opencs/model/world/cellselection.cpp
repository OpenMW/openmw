#include "cellselection.hpp"

#include <cmath>
#include <stdexcept>
#include <limits>

CSMWorld::CellSelection::Iterator CSMWorld::CellSelection::begin() const
{
    return mCells.begin();
}

CSMWorld::CellSelection::Iterator CSMWorld::CellSelection::end() const
{
    return mCells.end();
}

bool CSMWorld::CellSelection::add (const CellCoordinates& coordinates)
{
    return mCells.insert (coordinates).second;
}

void CSMWorld::CellSelection::remove (const CellCoordinates& coordinates)
{
    mCells.erase (coordinates);
}

bool CSMWorld::CellSelection::has (const CellCoordinates& coordinates) const
{
    return mCells.find (coordinates)!=end();
}

int CSMWorld::CellSelection::getSize() const
{
    return mCells.size();
}

CSMWorld::CellCoordinates CSMWorld::CellSelection::getCentre() const
{
    if (mCells.empty())
        throw std::logic_error ("call of getCentre on empty cell selection");

    double x = 0;
    double y = 0;

    for (Iterator iter = begin(); iter!=end(); ++iter)
    {
        x += iter->getX();
        y += iter->getY();
    }

    x /= mCells.size();
    y /= mCells.size();

    Iterator closest = begin();
    double distance = std::numeric_limits<double>::max();

    for (Iterator iter (begin()); iter!=end(); ++iter)
    {
        double deltaX = x - iter->getX();
        double deltaY = y - iter->getY();

        double delta = std::sqrt (deltaX * deltaX + deltaY * deltaY);

        if (delta<distance)
        {
            distance = delta;
            closest = iter;
        }
    }

    return *closest;
}

void CSMWorld::CellSelection::move (int x, int y)
{
    Container moved;

    for (Iterator iter = begin(); iter!=end(); ++iter)
        moved.insert (iter->move (x, y));

    mCells.swap (moved);
}
