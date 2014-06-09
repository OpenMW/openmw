
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <QtGui/qevent.h>

#include <apps/opencs/model/world/tablemimedata.hpp>

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget* parent, CSMDoc::Document& document)
: WorldspaceWidget (document, parent)
{}

void CSVRender::PagedWorldspaceWidget::useViewHint (const std::string& hint)
{
    if (!hint.empty())
    {
        CSMWorld::CellSelection selection;

        if (hint[0]=='c')
        {
            // syntax: c:#x1 y1; #x2 y2 (number of coordinate pairs can be 0 or larger)
            char ignore;

            std::istringstream stream (hint.c_str());
            if (stream >> ignore)
            {
                char ignore1; // : or ;
                char ignore2; // #
                int x, y;

                while (stream >> ignore1 >> ignore2 >> x >> y)
                    selection.add (CSMWorld::CellCoordinates (x, y));

                /// \todo adjust camera position
            }
        }
        else if (hint[0]=='r')
        {
            /// \todo implement 'r' type hints
        }

        setCellSelection (selection);
    }
}

void CSVRender::PagedWorldspaceWidget::setCellSelection (const CSMWorld::CellSelection& selection)
{
    mSelection = selection;
    emit cellSelectionChanged (mSelection);
}

std::pair< int, int > CSVRender::PagedWorldspaceWidget::getCoordinatesFromId (const std::string& record) const
{
    std::istringstream stream (record.c_str());
    char ignore;
    int x, y;
    stream >> ignore >> x >> y;
    return std::make_pair(x, y);
}

void CSVRender::PagedWorldspaceWidget::handleDrop (const std::vector< CSMWorld::UniversalId >& data)
{
    bool selectionChanged = false;
    for (unsigned i = 0; i < data.size(); ++i)
    {
        std::pair<int, int> coordinates(getCoordinatesFromId(data[i].getId()));
        if (mSelection.add(CSMWorld::CellCoordinates(coordinates.first, coordinates.second)))
        {
            selectionChanged = true;
        }
    }
    if (selectionChanged)
    {
        emit cellSelectionChanged(mSelection);
    }
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::PagedWorldspaceWidget::getDropRequirements (CSVRender::WorldspaceWidget::dropType type) const
{
    switch (type)
    {
        case cellsExterior:
            return canHandle;

        case cellsInterior:
            return needUnpaged;

        default:
            return ignored;
    }
}