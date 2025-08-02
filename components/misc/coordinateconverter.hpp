#ifndef OPENMW_COMPONENTS_MISC_COORDINATECONVERTER_H
#define OPENMW_COMPONENTS_MISC_COORDINATECONVERTER_H

#include <components/esm/esmbridge.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadpgrd.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/misc/constants.hpp>

namespace Misc
{
    /// \brief convert coordinates between world and local cell
    class CoordinateConverter
    {
    public:
        explicit CoordinateConverter(int cellX, int cellY)
            : mCellX(cellX)
            , mCellY(cellY)
        {
        }

        /// in-place conversion from local to world
        void toWorld(ESM::Pathgrid::Point& point) const
        {
            point.mX += mCellX;
            point.mY += mCellY;
        }

        /// in-place conversion from world to local
        void toLocal(ESM::Pathgrid::Point& point) const
        {
            point.mX -= mCellX;
            point.mY -= mCellY;
        }

        ESM::Pathgrid::Point toWorldPoint(ESM::Pathgrid::Point point) const
        {
            toWorld(point);
            return point;
        }

        ESM::Pathgrid::Point toLocalPoint(ESM::Pathgrid::Point point) const
        {
            toLocal(point);
            return point;
        }

        /// in-place conversion from local to world
        void toWorld(osg::Vec3f& point) const
        {
            point.x() += static_cast<float>(mCellX);
            point.y() += static_cast<float>(mCellY);
        }

        /// in-place conversion from world to local
        void toLocal(osg::Vec3f& point) const
        {
            point.x() -= static_cast<float>(mCellX);
            point.y() -= static_cast<float>(mCellY);
        }

        osg::Vec3f toWorldVec3(const osg::Vec3f& point) const
        {
            osg::Vec3f result = point;
            toWorld(result);
            return result;
        }

        osg::Vec3f toLocalVec3(const osg::Vec3f& point) const
        {
            osg::Vec3f result = point;
            toLocal(result);
            return result;
        }

    private:
        int mCellX;
        int mCellY;
    };

    template <class T>
    CoordinateConverter makeCoordinateConverterImpl(const T& cell)
    {
        if (cell.isExterior())
            return CoordinateConverter(cell.sSize * cell.getGridX(), cell.sSize * cell.getGridY());
        return CoordinateConverter(0, 0);
    }

    inline CoordinateConverter makeCoordinateConverter(const ESM::Cell& cell)
    {
        return makeCoordinateConverterImpl(cell);
    }

    inline CoordinateConverter makeCoordinateConverter(const ESM4::Cell& cell)
    {
        return makeCoordinateConverterImpl(cell);
    }

    inline CoordinateConverter makeCoordinateConverter(const ESM::CellVariant& cell)
    {
        return visit([](const auto& v) { return makeCoordinateConverterImpl(v); }, cell);
    }
}

#endif
