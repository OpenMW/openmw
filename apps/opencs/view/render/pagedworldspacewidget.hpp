#ifndef OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H

#include <map>

#include "../../model/world/cellselection.hpp"

#include "worldspacewidget.hpp"
#include "cell.hpp"

namespace CSVWidget
{
   class SceneToolToggle;
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
            CSVWidget::SceneToolToggle *mControlElements;
            bool mDisplayCellCoord;

        private:

            std::pair<int, int> getCoordinatesFromId(const std::string& record) const;

            /// Bring mCells into sync with mSelection again.
            ///
            /// \return Any cells added or removed?
            bool adjustCells();

            virtual void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            virtual void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void referenceableAdded (const QModelIndex& index, int start, int end);

            virtual void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void referenceAdded (const QModelIndex& index, int start, int end);

            virtual std::string getStartupInstruction();

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

        public:

            PagedWorldspaceWidget (QWidget *parent, CSMDoc::Document& document);
            ///< \note Sets the cell area selection to an invalid value to indicate that currently
            /// no cells are displayed. The cells to be displayed will be specified later through
            /// hint system.

            virtual ~PagedWorldspaceWidget();

            void useViewHint (const std::string& hint);

            void setCellSelection(const CSMWorld::CellSelection& selection);

            /// \return Drop handled?
            virtual bool handleDrop (const std::vector<CSMWorld::UniversalId>& data,
                DropType type);

            virtual dropRequirments getDropRequirements(DropType type) const;

            /// \attention The created tool is not added to the toolbar (via addTool). Doing
            /// that is the responsibility of the calling function.
            virtual CSVWidget::SceneToolToggle *makeControlVisibilitySelector (
                CSVWidget::SceneToolbar *parent);

            virtual unsigned int getVisibilityMask() const;

            /// \param elementMask Elements to be affected by the clear operation
            virtual void clearSelection (int elementMask);

        protected:

            virtual void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle2 *tool);

            virtual void addEditModeSelectorButtons (CSVWidget::SceneToolMode *tool);

            virtual void handleMouseClick (osg::ref_ptr<TagBase> tag, const std::string& button, bool shift);

        signals:

            void cellSelectionChanged (const CSMWorld::CellSelection& selection);

        private slots:

            virtual void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void cellRemoved (const QModelIndex& parent, int start, int end);

            virtual void cellAdded (const QModelIndex& index, int start, int end);

    };
}

#endif
