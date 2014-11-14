#ifndef OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H

#include <map>

#include "../../model/world/cellselection.hpp"

#include "worldspacewidget.hpp"
#include "cell.hpp"

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
            std::map<CSMWorld::CellCoordinates, TextOverlay *> mTextOverlays;
            OverlayMask *mOverlayMask;

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

            //virtual void pathgridAdded (const QModelIndex& parent, int start, int end);

            //virtual void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            //virtual void pathgridAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual std::string getStartupInstruction();

            Cell *findCell(const std::string &cellId);

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

        protected:

            virtual void addEditModeSelectorButtons (CSVWidget::SceneToolMode *tool);

            virtual void updateOverlay();

            virtual void mousePressEvent (QMouseEvent *event);

            virtual void mouseReleaseEvent (QMouseEvent *event);

            virtual void mouseDoubleClickEvent (QMouseEvent *event);

            // FIXME: temporary only until signals from the document is implemented
            virtual void pathgridInserted (const std::string &referenceId, const Ogre::Vector3 &pos);
            virtual void pathgridMoved (const std::string &pgName, const Ogre::Vector3 &pos);
            virtual void pathgridAboutToBeRemoved (const std::string &pgName);

        signals:

            void cellSelectionChanged (const CSMWorld::CellSelection& selection);

        private slots:

            virtual void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void cellRemoved (const QModelIndex& parent, int start, int end);

            virtual void cellAdded (const QModelIndex& index, int start, int end);

    };
}

#endif
