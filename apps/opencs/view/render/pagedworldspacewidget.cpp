
#include "pagedworldspacewidget.hpp"

#include <sstream>

#include <qt4/QtGui/qevent.h>

#include <boost/algorithm/string.hpp>

#include <apps/opencs/model/world/tablemimedata.hpp>

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget *parent, const CSMDoc::Document& document)
: WorldspaceWidget (parent),
mDocument(document)
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
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData());
    if (mime->fromDocument(mDocument))
    {
        std::vector<CSMWorld::UniversalId> data(mime->getData());

        for (unsigned i = 0; i < data.size(); ++i)
        {
            if (data[i].getType() == CSMWorld::UniversalId::Type_Cell ||
                data[i].getType() == CSMWorld::UniversalId::Type_Cell_Missing)
            {
                if (*(data[i].getId().begin()) == '#')
                {
                    std::pair<int, int> coordinate(getCoordinatesFromId(data[i].getId()));
                    mSelection.add(CSMWorld::CellCoordinates(coordinate.first, coordinate.second));
                }
            }
        }
    }
}

std::pair< int, int > CSVRender::PagedWorldspaceWidget::getCoordinatesFromId (const std::string& record) const
{
    QString id(QString::fromUtf8(record.c_str()));
    id.remove(0,1);
    QStringList splited(id.split(' ')); //Well, this is the simplest approach
    int x = splited.begin()->toInt();
    int y = (splited.begin()+1)->toInt();
    return std::make_pair(x, y);
}

