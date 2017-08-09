#ifndef OPENCS_VIEW_CELLMARKER_H
#define OPENCS_VIEW_CELLMARKER_H

#include "tagbase.hpp"

#include <osg/ref_ptr>

#include "../../model/world/cellcoordinates.hpp"

namespace osg
{
    class AutoTransform;
    class Group;
}

namespace CSVRender
{
    class CellMarker;

    class CellMarkerTag : public TagBase
    {
        private:

            CellMarker *mMarker;

        public:

            CellMarkerTag(CellMarker *marker);

            CellMarker *getCellMarker() const;
    };

    /// \brief Marker to display cell coordinates.
    class CellMarker
    {
        private:

            osg::Group* mCellNode;
            osg::ref_ptr<osg::AutoTransform> mMarkerNode;
            CSMWorld::CellCoordinates mCoordinates;
            bool mExists;

            // Not implemented.
            CellMarker(const CellMarker&);
            CellMarker& operator=(const CellMarker&);

            /// \brief Build marker containing cell's coordinates.
            void buildMarker();

            /// \brief Position marker at center of cell.
            void positionMarker();

        public:

            /// \brief Constructor.
            /// \param cellNode Cell to create marker for.
            /// \param coordinates Coordinates of cell.
            /// \param cellExists Whether or not cell exists.
            CellMarker(
                osg::Group *cellNode,
                const CSMWorld::CellCoordinates& coordinates,
                const bool cellExists);

            ~CellMarker();
    };
}

#endif
