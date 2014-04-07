
#include "pagedworldspacewidget.hpp"

#include <sstream>

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget *parent)
: WorldspaceWidget (parent)
{}

void CSVRender::PagedWorldspaceWidget::useViewHint (const std::string& hint)
{
    if (!hint.empty())
    {
        CSMWorld::CellSelection selection;

        if (hint[0]=='c')
        {
            char ignore1, ignore2, ignore3;
            int x, y;

            std::istringstream stream (hint.c_str());
            if (stream >> ignore1 >> ignore2 >> ignore3 >> x >> y)
            {
                selection.add (CSMWorld::CellCoordinates (x, y));

                /// \todo adjust camera position
            }
        }

        /// \todo implement 'r' type hints

        setCellSelection (selection);
    }
}

void CSVRender::PagedWorldspaceWidget::setCellSelection (const CSMWorld::CellSelection& selection)
{
    mSelection = selection;
    emit cellSelectionChanged (mSelection);
}