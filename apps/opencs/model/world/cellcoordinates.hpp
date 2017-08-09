#ifndef CSM_WOLRD_CELLCOORDINATES_H
#define CSM_WOLRD_CELLCOORDINATES_H

#include <iosfwd>
#include <string>
#include <utility>

#include <QMetaType>

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
    };

    bool operator== (const CellCoordinates& left, const CellCoordinates& right);
    bool operator!= (const CellCoordinates& left, const CellCoordinates& right);
    bool operator< (const CellCoordinates& left, const CellCoordinates& right);

    std::ostream& operator<< (std::ostream& stream, const CellCoordinates& coordiantes);
}

Q_DECLARE_METATYPE (CSMWorld::CellCoordinates)

#endif
