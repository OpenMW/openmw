#ifndef OPENCS_VIEW_CELL_H
#define OPENCS_VIEW_CELL_H

#include <string>
#include <map>
#include <memory>
#include <vector>

#include <osg/ref_ptr>

#include "../../model/world/cellcoordinates.hpp"
#include "terrainstorage.hpp"

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
}

namespace Terrain
{
    class TerrainGrid;
}

namespace CSVRender
{
    class CellWater;
    class Pathgrid;
    class TagBase;
    class Object;

    class CellArrow;
    class CellBorder;
    class CellMarker;
    class CellWater;

    class Cell
    {
            CSMWorld::Data& mData;
            std::string mId;
            osg::ref_ptr<osg::Group> mCellNode;
            std::map<std::string, Object *> mObjects;
            std::unique_ptr<Terrain::TerrainGrid> mTerrain;
            CSMWorld::CellCoordinates mCoordinates;
            std::unique_ptr<CellArrow> mCellArrows[4];
            std::unique_ptr<CellMarker> mCellMarker;
            std::unique_ptr<CellBorder> mCellBorder;
            std::unique_ptr<CellWater> mCellWater;
            std::unique_ptr<Pathgrid> mPathgrid;
            bool mDeleted;
            int mSubMode;
            unsigned int mSubModeElementMask;
            bool mUpdateLand, mLandDeleted;
            TerrainStorage *mTerrainStorage;

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

            void updateLand();
            void unloadLand();

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

            void setAlteredHeight(int inCellX, int inCellY, float height);

            float getSumOfAlteredAndTrueHeight(int cellX, int cellY, int inCellX, int inCellY);

            float* getAlteredHeight(int inCellX, int inCellY);

            void resetAlteredHeights();

            void pathgridModified();

            void pathgridRemoved();

            void landDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void landAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void landAdded (const QModelIndex& parent, int start, int end);

            void landTextureChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void landTextureAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void landTextureAdded (const QModelIndex& parent, int start, int end);

            void reloadAssets();

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

            friend class CellNodeCallback;
    };
}

#endif
