#ifndef OPENCS_VIEW_CELLARROW_H
#define OPENCS_VIEW_CELLARROW_H

#include "tagbase.hpp"

#include <osg/ref_ptr>

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
            int mXIndex;
            int mYIndex;

            void adjustTransform();

            void buildShape();

        public:

            CellArrow (osg::Group *cellNode, Direction direction, int xIndex, int yIndex);

            ~CellArrow();

            int getXIndex() const;

            int getYIndex() const;
    };
}

#endif
