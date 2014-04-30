#ifndef OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H

#include "../../model/world/cellselection.hpp"
#include <apps/opencs/model/doc/document.hpp>

#include "worldspacewidget.hpp"

namespace CSVRender
{
    class PagedWorldspaceWidget : public WorldspaceWidget
    {
            Q_OBJECT

            CSMWorld::CellSelection mSelection;
            const CSMDoc::Document& mDocument; //for checking if drop comes from same document

        private:

            void dropEvent(QDropEvent* event);

            void dragEnterEvent(QDragEnterEvent *event);

            void dragMoveEvent(QDragMoveEvent *event);

            std::pair<int, int> getCoordinatesFromId(const std::string& record) const;

        public:

            PagedWorldspaceWidget (QWidget *parent, const CSMDoc::Document& document);
            ///< \note Sets the cell area selection to an invalid value to indicate that currently
            /// no cells are displayed. The cells to be displayed will be specified later through
            /// hint system.

            virtual void useViewHint (const std::string& hint);

            void setCellSelection (const CSMWorld::CellSelection& selection);

        signals:

            void cellSelectionChanged (const CSMWorld::CellSelection& selection);
    };
}

#endif
