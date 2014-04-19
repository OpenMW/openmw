#ifndef CSM_WOLRD_CELLCOORDINATES_H
#define CSM_WOLRD_CELLCOORDINATES_H

#include <iosfwd>
#include <string>

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

            int getX() const;

            int getY() const;

            CellCoordinates move (int x, int y) const;
            ///< Return a copy of *this, moved by the given offset.

            std::string getId (const std::string& worldspace) const;
            ///< Return the ID for the cell at these coordinates.
    };

    bool operator== (const CellCoordinates& left, const CellCoordinates& right);
    bool operator!= (const CellCoordinates& left, const CellCoordinates& right);
    bool operator< (const CellCoordinates& left, const CellCoordinates& right);

    std::ostream& operator<< (std::ostream& stream, const CellCoordinates& coordiantes);
}

Q_DECLARE_METATYPE (CSMWorld::CellCoordinates)

#endif
