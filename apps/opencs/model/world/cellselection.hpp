#ifndef CSM_WOLRD_CELLSELECTION_H
#define CSM_WOLRD_CELLSELECTION_H

#include <set>

#include <QMetaType>

#include "cellcoordinates.hpp"

namespace CSMWorld
{
    /// \brief Selection of cells in a paged worldspace
    ///
    /// \note The CellSelection does not specify the worldspace it applies to.
    class CellSelection
    {
        public:

            typedef std::set<CellCoordinates> Container;
            typedef Container::const_iterator Iterator;

        private:

            Container mCells;

        public:

            Iterator begin() const;

            Iterator end() const;

            bool add (const CellCoordinates& coordinates);
            ///< Ignored if the cell specified by \a coordinates is already part of the selection.
            ///
            /// \return Was a cell added to the collection?

            void remove (const CellCoordinates& coordinates);
            ///< ignored if the cell specified by \a coordinates is not part of the selection.

            bool has (const CellCoordinates& coordinates) const;
            ///< \return Is the cell specified by \a coordinates part of the selection?

            int getSize() const;
            ///< Return number of cells.

            CellCoordinates getCentre() const;
            ///< Return the selected cell that is closest to the geometric centre of the selection.
            ///
            /// \attention This function must not be called on selections that are empty.

            void move (int x, int y);
    };
}

Q_DECLARE_METATYPE (CSMWorld::CellSelection)

#endif
