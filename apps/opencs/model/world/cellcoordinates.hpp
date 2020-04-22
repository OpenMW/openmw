#ifndef CSM_WOLRD_CELLCOORDINATES_H
#define CSM_WOLRD_CELLCOORDINATES_H

#include <iosfwd>
#include <string>
#include <utility>

#include <QMetaType>

#include <osg/Vec3d>

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

            ///Generate cell id string from x and y coordinates
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

            ///Converts worldspace coordinates to global texture selection, taking in account the texture offset.
            static std::pair<int, int> toTextureCoords(const osg::Vec3d& worldPos);

            ///Converts worldspace coordinates to global vertex selection.
            static std::pair<int, int> toVertexCoords(const osg::Vec3d& worldPos);

            ///Converts global texture coordinate X to worldspace coordinate, offset by 0.25f.
            static float textureGlobalXToWorldCoords(int textureGlobal);

            ///Converts global texture coordinate Y to worldspace coordinate, offset by 0.25f.
            static float textureGlobalYToWorldCoords(int textureGlobal);

            ///Converts global vertex coordinate to worldspace coordinate
            static float vertexGlobalToWorldCoords(int vertexGlobal);

            ///Converts global vertex coordinate to local cell's heightmap coordinates
            static int vertexGlobalToInCellCoords(int vertexGlobal);

            ///Converts global texture coordinates to cell id
            static std::string textureGlobalToCellId(const std::pair<int, int>& textureGlobal);

            ///Converts global vertex coordinates to cell id
            static std::string vertexGlobalToCellId(const std::pair<int, int>& vertexGlobal);
    };

    bool operator== (const CellCoordinates& left, const CellCoordinates& right);
    bool operator!= (const CellCoordinates& left, const CellCoordinates& right);
    bool operator< (const CellCoordinates& left, const CellCoordinates& right);

    std::ostream& operator<< (std::ostream& stream, const CellCoordinates& coordiantes);
}

Q_DECLARE_METATYPE (CSMWorld::CellCoordinates)

#endif
