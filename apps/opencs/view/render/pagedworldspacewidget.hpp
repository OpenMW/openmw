#ifndef OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H

#include "../../model/world/cellselection.hpp"

#include "worldspacewidget.hpp"

namespace CSVRender
{
    class PagedWorldspaceWidget : public WorldspaceWidget
    {
            Q_OBJECT

            CSMWorld::CellSelection mSelection;

        public:

            PagedWorldspaceWidget (QWidget *parent);
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
