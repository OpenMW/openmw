#ifndef OPENCS_VIEW_CELLARROW_H
#define OPENCS_VIEW_CELLARROW_H

#include "tagbase.hpp"

#include <osg/ref_ptr>

#include "../../model/world/cellcoordinates.hpp"

namespace osg
{
    class PositionAttitudeTransform;
    class Group;
}

namespace CSVRender
{
    class CellArrow;

    class CellArrowTag : public TagBase
    {
            CellArrow *mArrow;

        public:

            CellArrowTag (CellArrow *arrow);

            CellArrow *getCellArrow() const;

            QString getToolTip(bool hideBasics, const WorldspaceHitResult& hit) const override;
    };


    class CellArrow
    {
        public:

            enum Direction
            {
                Direction_North = 1,
                Direction_West = 2,
                Direction_South = 4,
                Direction_East = 8
            };

        private:

            // not implemented
            CellArrow (const CellArrow&);
            CellArrow& operator= (const CellArrow&);

            Direction mDirection;
            osg::Group* mParentNode;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
            CSMWorld::CellCoordinates mCoordinates;

            void adjustTransform();

            void buildShape();

        public:

            CellArrow (osg::Group *cellNode, Direction direction,
                const CSMWorld::CellCoordinates& coordinates);

            ~CellArrow();

            CSMWorld::CellCoordinates getCoordinates() const;

            Direction getDirection() const;
    };
}

#endif
