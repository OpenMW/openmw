#include "coordinateconverter.hpp"

#include <components/esm/loadcell.hpp>
#include <components/esm/loadland.hpp>

namespace MWMechanics
{
    CoordinateConverter::CoordinateConverter(const ESM::Cell* cell)
        : mCellX(0), mCellY(0)
    {
        if (cell->isExterior())
        {
            mCellX = cell->mData.mX * ESM::Land::REAL_SIZE;
            mCellY = cell->mData.mY * ESM::Land::REAL_SIZE;
        }
    }

    void CoordinateConverter::toWorld(ESM::Pathgrid::Point& point)
    {
        point.mX += mCellX;
        point.mY += mCellY;
    }

    void CoordinateConverter::toWorld(osg::Vec3f& point)
    {
        point.x() += static_cast<float>(mCellX);
        point.y() += static_cast<float>(mCellY);
    }

    void CoordinateConverter::toLocal(osg::Vec3f& point)
    {
        point.x() -= static_cast<float>(mCellX);
        point.y() -= static_cast<float>(mCellY);
    }

    osg::Vec3f CoordinateConverter::toLocalVec3(const osg::Vec3f& point)
    {
        return osg::Vec3f(
            point.x() - static_cast<float>(mCellX),
            point.y() - static_cast<float>(mCellY),
            point.z()
        );
    }
}
