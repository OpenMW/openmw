#ifndef OPENCS_VIEW_CELL_H
#define OPENCS_VIEW_CELL_H

#include <string>
#include <map>
#include <memory>

#include <boost/shared_ptr.hpp>

#include <osg/ref_ptr>

#ifndef Q_MOC_RUN
#include <components/terrain/terraingrid.hpp>
#endif

#include "object.hpp"
#include "cellarrow.hpp"

class QModelIndex;

namespace osg
{
    class Group;
}

namespace CSMWorld
{
    class Data;
    class CellCoordinates;
}

namespace CSVRender
{
    class Cell
    {
            CSMWorld::Data& mData;
            std::string mId;
            osg::ref_ptr<osg::Group> mCellNode;
            std::map<std::string, Object *> mObjects;
            std::auto_ptr<Terrain::TerrainGrid> mTerrain;
            CSMWorld::CellCoordinates mCoordinates;
            std::auto_ptr<CellArrow> mCellArrows[4];
            bool mDeleted;

            /// Ignored if cell does not have an object with the given ID.
            ///
            /// \return Was the object deleted?
            bool removeObject (const std::string& id);

            /// Add objects from reference table that are within this cell.
            ///
            /// \return Have any objects been added?
            bool addObjects (int start, int end);

        public:

            enum Selection
            {
                Selection_Clear,
                Selection_All,
                Selection_Invert
            };

        public:

            /// \note Deleted covers both cells that are deleted and cells that don't exist in
            /// the first place.
            Cell (CSMWorld::Data& data, osg::Group* rootNode, const std::string& id,
                bool deleted = false);

            ~Cell();

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceAdded (const QModelIndex& parent, int start, int end);

            void setSelection (int elementMask, Selection mode);

            void setCellArrows (int mask);

            /// Returns 0, 0 in case of an unpaged cell.
            CSMWorld::CellCoordinates getCoordinates() const;

            bool isDeleted() const;
    };
}

#endif
