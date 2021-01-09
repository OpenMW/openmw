#ifndef OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H

#include <map>

#include "../../model/world/cellselection.hpp"

#include "worldspacewidget.hpp"
#include "cell.hpp"
#include "instancedragmodes.hpp"

namespace CSVWidget
{
   class SceneToolToggle;
   class SceneToolToggle2;
}

namespace CSVRender
{
    class TextOverlay;
    class OverlayMask;

    class PagedWorldspaceWidget : public WorldspaceWidget
    {
            Q_OBJECT

            CSMDoc::Document& mDocument;
            CSMWorld::CellSelection mSelection;
            std::map<CSMWorld::CellCoordinates, Cell *> mCells;
            std::string mWorldspace;
            CSVWidget::SceneToolToggle2 *mControlElements;
            bool mDisplayCellCoord;

        private:

            std::pair<int, int> getCoordinatesFromId(const std::string& record) const;

            /// Bring mCells into sync with mSelection again.
            ///
            /// \return Any cells added or removed?
            bool adjustCells();

            void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight) override;

            void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end) override;

            void referenceableAdded (const QModelIndex& index, int start, int end) override;

            void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) override;

            void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end) override;

            void referenceAdded (const QModelIndex& index, int start, int end) override;

            void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight) override;

            void pathgridAboutToBeRemoved (const QModelIndex& parent, int start, int end) override;

            void pathgridAdded (const QModelIndex& parent, int start, int end) override;

            std::string getStartupInstruction() override;

            /// \note Does not update the view or any cell marker
            void addCellToScene (const CSMWorld::CellCoordinates& coordinates);

            /// \note Does not update the view or any cell marker
            ///
            /// \note Calling this function for a cell that is not in the selection is a no-op.
            void removeCellFromScene (const CSMWorld::CellCoordinates& coordinates);

            /// \note Does not update the view or any cell marker
            void addCellSelection (int x, int y);

            /// \note Does not update the view or any cell marker
            void moveCellSelection (int x, int y);

            void addCellToSceneFromCamera (int offsetX, int offsetY);

        public:

            PagedWorldspaceWidget (QWidget *parent, CSMDoc::Document& document);
            ///< \note Sets the cell area selection to an invalid value to indicate that currently
            /// no cells are displayed. The cells to be displayed will be specified later through
            /// hint system.

            virtual ~PagedWorldspaceWidget();

            /// Decodes the the hint string to set of cell that are rendered.
            void useViewHint (const std::string& hint) override;

            void setCellSelection(const CSMWorld::CellSelection& selection);

            const CSMWorld::CellSelection& getCellSelection() const;

            /// \return Drop handled?
            bool handleDrop (const std::vector<CSMWorld::UniversalId>& data,
                DropType type) override;

            dropRequirments getDropRequirements(DropType type) const override;

            /// \attention The created tool is not added to the toolbar (via addTool). Doing
            /// that is the responsibility of the calling function.
            virtual CSVWidget::SceneToolToggle2 *makeControlVisibilitySelector (
                CSVWidget::SceneToolbar *parent);

            unsigned int getVisibilityMask() const override;

            /// \param elementMask Elements to be affected by the clear operation
            void clearSelection (int elementMask) override;

            /// \param elementMask Elements to be affected by the select operation
            void invertSelection (int elementMask) override;

            /// \param elementMask Elements to be affected by the select operation
            void selectAll (int elementMask) override;

            // Select everything that references the same ID as at least one of the elements
            // already selected
            //
            /// \param elementMask Elements to be affected by the select operation
            void selectAllWithSameParentId (int elementMask) override;

            void selectInsideCube(const osg::Vec3d& pointA, const osg::Vec3d& pointB, DragMode dragMode) override;

            void selectWithinDistance(const osg::Vec3d& point, float distance, DragMode dragMode) override;

            std::string getCellId (const osg::Vec3f& point) const override;

            Cell* getCell(const osg::Vec3d& point) const override;

            Cell* getCell(const CSMWorld::CellCoordinates& coords) const override;

            void setCellAlteredHeight(const CSMWorld::CellCoordinates& coords, int inCellX, int inCellY, float height);

            float* getCellAlteredHeight(const CSMWorld::CellCoordinates& coords, int inCellX, int inCellY);

            void resetAllAlteredHeights();

            std::vector<osg::ref_ptr<TagBase> > getSelection (unsigned int elementMask)
                const override;

            std::vector<osg::ref_ptr<TagBase> > getEdited (unsigned int elementMask)
                const override;

            void setSubMode (int subMode, unsigned int elementMask) override;

            /// Erase all overrides and restore the visual representation to its true state.
            void reset (unsigned int elementMask) override;

        protected:

            void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle2 *tool) override;

            void addEditModeSelectorButtons (CSVWidget::SceneToolMode *tool) override;

            void handleInteractionPress (const WorldspaceHitResult& hit, InteractionType type) override;

        signals:

            void cellSelectionChanged (const CSMWorld::CellSelection& selection);

        private slots:

            virtual void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void cellRemoved (const QModelIndex& parent, int start, int end);

            virtual void cellAdded (const QModelIndex& index, int start, int end);

            virtual void landDataChanged (const QModelIndex& topLeft, const QModelIndex& botomRight);
            virtual void landAboutToBeRemoved (const QModelIndex& parent, int start, int end);
            virtual void landAdded (const QModelIndex& parent, int start, int end);

            virtual void landTextureDataChanged (const QModelIndex& topLeft, const QModelIndex& botomRight);
            virtual void landTextureAboutToBeRemoved (const QModelIndex& parent, int start, int end);
            virtual void landTextureAdded (const QModelIndex& parent, int start, int end);

            void assetTablesChanged ();

            void loadCameraCell();

            void loadEastCell();

            void loadNorthCell();

            void loadWestCell();

            void loadSouthCell();

    };
}

#endif
