#include "droplineedit.hpp"

#include <QDropEvent>

#include "../../model/world/tablemimedata.hpp"

namespace
{
    const CSMWorld::TableMimeData *getEventMimeData(QDropEvent *event)
    {
        Q_ASSERT(event != NULL);
        return dynamic_cast<const CSMWorld::TableMimeData *>(event->mimeData());
    }
}

CSVWidget::DropLineEdit::DropLineEdit(CSMWorld::UniversalId::Type type, QWidget *parent) 
    : QLineEdit(parent),
      mDropType(type)
{
    setAcceptDrops(true);
}

CSVWidget::DropLineEdit::DropLineEdit(CSMWorld::ColumnBase::Display display, QWidget *parent)
    : QLineEdit(parent),
      mDropType(CSMWorld::TableMimeData::convertEnums(display))
{
    setAcceptDrops(true);
}

void CSVWidget::DropLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (canAcceptEventData(event))
    {
        event->acceptProposedAction();
    }
}

void CSVWidget::DropLineEdit::dragMoveEvent(QDragMoveEvent *event)
{
    if (canAcceptEventData(event))
    {
        event->accept();
    }
}

void CSVWidget::DropLineEdit::dropEvent(QDropEvent *event)
{
    const CSMWorld::TableMimeData *data = getEventMimeData(event);
    if (data == NULL) // May happen when non-records (e.g. plain text) are dragged and dropped
    {
        return;
    }

    int dataIndex = getAcceptedDataIndex(*data);
    if (dataIndex != -1)
    {
        std::vector<CSMWorld::UniversalId> idData = data->getData();
        setText(idData[dataIndex].getId().c_str());
        emit tableMimeDataDropped(idData[dataIndex], data->getDocumentPtr());
    }
}

bool CSVWidget::DropLineEdit::canAcceptEventData(QDropEvent *event) const
{
    const CSMWorld::TableMimeData *data = getEventMimeData(event);
    if (data == NULL) // May happen when non-records (e.g. plain text) are dragged and dropped
    {
        return false;
    }
    return getAcceptedDataIndex(*data) != -1;
}

int CSVWidget::DropLineEdit::getAcceptedDataIndex(const CSMWorld::TableMimeData &data) const
{
    if (mDropType == CSMWorld::UniversalId::Type_None)
    {
        return 0;
    }

    bool isReferenceable = mDropType == CSMWorld::UniversalId::Type_Referenceable;
    std::vector<CSMWorld::UniversalId> idData = data.getData();
    int size = static_cast<int>(idData.size());
    for (int i = 0; i < size; ++i)
    {
        CSMWorld::UniversalId::Type type = idData[i].getType();
        if (type == mDropType || isReferenceable && CSMWorld::TableMimeData::isReferencable(type))
        {
            return i;
        }
    }
    return -1;
}
