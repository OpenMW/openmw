#ifndef CSM_WOLRD_CELLCOORDINATES_H
#define CSM_WOLRD_CELLCOORDINATES_H

#include <iosfwd>
#include <string>
#include <utility>

#include <QMetaType>

//#include <osg/Group>
//#include <osg/Geometry>
#include <osg/PositionAttitudeTransform>

namespace CSMWorld
{
    class CellCoordinates
    {
            int mX;
            int mY;

        public:

            CellCoordinates();

            CellCoordinates (int x, int y);

            CellCoordinates (const std::pair<int, int>& coordinates);

            int getX() const;

            int getY() const;

            CellCoordinates move (int x, int y) const;
            ///< Return a copy of *this, moved by the given offset.

            static std::string generateId (int x, int y);

            std::string getId (const std::string& worldspace) const;
            ///< Return the ID for the cell at these coordinates.

            static bool isExteriorCell (const std::string& id);

            /// \return first: CellCoordinates (or 0, 0 if cell does not have coordinates),
            /// second: is cell paged?
            ///
            /// \note The worldspace part of \a id is ignored
            static std::pair<CellCoordinates, bool> fromId (const std::string& id);

            /// \return cell coordinates such that given world coordinates are in it.
            static std::pair<int, int> coordinatesToCellIndex (float x, float y);

            ///Converts Worldspace coordinates to global texture selection, modified by texture offset.
            static std::pair<int, int> toTextureCoords(osg::Vec3d worldPos);

            ///Converts Worldspace coordinates to global vertex selection.
            static std::pair<int, int> toVertexCoords(osg::Vec3d worldPos);

            ///Converts Global texture coordinates to Worldspace coordinate at upper left corner of the selected texture.
            static double texSelectionToWorldCoords(int);
    };

    bool operator== (const CellCoordinates& left, const CellCoordinates& right);
    bool operator!= (const CellCoordinates& left, const CellCoordinates& right);
    bool operator< (const CellCoordinates& left, const CellCoordinates& right);

    std::ostream& operator<< (std::ostream& stream, const CellCoordinates& coordiantes);
}

Q_DECLARE_METATYPE (CSMWorld::CellCoordinates)

#endif
