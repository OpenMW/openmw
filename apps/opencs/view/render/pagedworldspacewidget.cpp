
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <qt4/QtGui/qevent.h>

#include <boost/algorithm/string.hpp>

#include <apps/opencs/model/world/tablemimedata.hpp>

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget *parent)
: WorldspaceWidget (parent)
{
    setAcceptDrops(true);
}

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

void CSVRender::PagedWorldspaceWidget::dragEnterEvent (QDragEnterEvent* event)
{
    event->accept();
}

void CSVRender::PagedWorldspaceWidget::dragMoveEvent (QDragMoveEvent* event)
{
    event->accept();
}

void CSVRender::PagedWorldspaceWidget::dropEvent (QDropEvent* event)
{
    /*
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (true)
    {
        if (mime->holdsType(CSMWorld::UniversalId::Type_Cell))
        {
            CSMWorld::UniversalId record(mime->returnMatching (CSMWorld::UniversalId::Type_Cell));
            QString id(QString::fromUtf8(record.getId().c_str()));
            if (*id.begin() == '#')
            {
                id.remove(0,1);
                QStringList splited(id.split(' '));
                int x = splited.begin()->toInt();
                int y = (splited.begin()+1)->toInt();
                mSelection.add(CSMWorld::CellCoordinates(x, y));
            }
        }
    }
    */
    //TODO!
}
