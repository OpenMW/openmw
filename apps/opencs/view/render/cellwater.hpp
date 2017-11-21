#ifndef CSV_RENDER_CELLWATER_H
#define CSV_RENDER_CELLWATER_H

#include <string>

#include <osg/ref_ptr>

#include <QObject>
#include <QModelIndex>

#include "../../model/world/record.hpp"

namespace osg
{
    class Geode;
    class Geometry;
    class Group;
    class PositionAttitudeTransform;
}

namespace CSMWorld
{
    struct Cell;
    class CellCoordinates;
    class Data;
}

namespace CSVRender
{
    /// For exterior cells, this adds a patch of water to fit the size of the cell. For interior cells with water, this
    /// adds a large patch of water much larger than the typical size of a cell.
    class CellWater : public QObject
    {
            Q_OBJECT

        public:

            CellWater(CSMWorld::Data& data, osg::Group* cellNode, const std::string& id,
                const CSMWorld::CellCoordinates& cellCoords);

            ~CellWater();

            void updateCellData(const CSMWorld::Record<CSMWorld::Cell>& cellRecord);

            void reloadAssets();

        private slots:

            void cellDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        private:

            void recreate();

            static const int CellSize;

            CSMWorld::Data& mData;
            std::string mId;

            osg::Group* mParentNode;

            osg::ref_ptr<osg::PositionAttitudeTransform> mWaterTransform;
            osg::ref_ptr<osg::Geode> mWaterNode;
            osg::ref_ptr<osg::Geometry> mWaterGeometry;

            bool mDeleted;
            bool mExterior;
            bool mHasWater;
    };
}

#endif
