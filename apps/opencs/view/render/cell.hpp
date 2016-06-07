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
#include "cellmarker.hpp"
#include "cellborder.hpp"

class QModelIndex;

namespace osg
{
    class Group;
    class Geometry;
    class Geode;
}

namespace CSMWorld
{
    class Data;
    class CellCoordinates;
}

namespace CSVRender
{
    class Pathgrid;
    class TagBase;

    class Cell
    {
            CSMWorld::Data& mData;
            std::string mId;
            osg::ref_ptr<osg::Group> mCellNode;
            std::map<std::string, Object *> mObjects;
            std::auto_ptr<Terrain::TerrainGrid> mTerrain;
            CSMWorld::CellCoordinates mCoordinates;
            std::auto_ptr<CellArrow> mCellArrows[4];
            std::auto_ptr<CellMarker> mCellMarker;
            std::auto_ptr<CellBorder> mCellBorder;
            std::auto_ptr<Pathgrid> mPathgrid;
            bool mDeleted;
            int mSubMode;
            unsigned int mSubModeElementMask;

            /// Ignored if cell does not have an object with the given ID.
            ///
            /// \return Was the object deleted?
            bool removeObject (const std::string& id);

            // Remove object and return iterator to next object.
            std::map<std::string, Object *>::iterator removeObject (
                std::map<std::string, Object *>::iterator iter);

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

            /// \note Returns the pathgrid representation which will exist as long as the cell exists
            Pathgrid* getPathgrid() const;

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

            void pathgridModified();

            void pathgridRemoved();

            void setSelection (int elementMask, Selection mode);

            // Select everything that references the same ID as at least one of the elements
            // already selected
            void selectAllWithSameParentId (int elementMask);

            void setCellArrows (int mask);

            /// \brief Set marker for this cell.
            void setCellMarker();

            /// Returns 0, 0 in case of an unpaged cell.
            CSMWorld::CellCoordinates getCoordinates() const;

            bool isDeleted() const;

            std::vector<osg::ref_ptr<TagBase> > getSelection (unsigned int elementMask) const;

            std::vector<osg::ref_ptr<TagBase> > getEdited (unsigned int elementMask) const;

            void setSubMode (int subMode, unsigned int elementMask);

            /// Erase all overrides and restore the visual representation of the cell to its
            /// true state.
            void reset (unsigned int elementMask);
    };
}

#endif
