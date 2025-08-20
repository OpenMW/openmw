#include "filterbox.hpp"

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <QDragEnterEvent>
#include <QHBoxLayout>
#include <QVariant>

#include "filterdata.hpp"
#include "recordfilterbox.hpp"

#include <apps/opencs/model/world/tablemimedata.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/world/dragrecordtable.hpp>

CSVFilter::FilterBox::FilterBox(CSMWorld::Data& worldData, QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);

    mRecordFilterBox = new RecordFilterBox(worldData, this);

    layout->addWidget(mRecordFilterBox);

    setLayout(layout);

    connect(mRecordFilterBox, &RecordFilterBox::filterChanged, this, &FilterBox::recordFilterChanged);

    setAcceptDrops(true);
}

void CSVFilter::FilterBox::setRecordFilter(const std::string& filter)
{
    mRecordFilterBox->setFilter(filter);
}

void CSVFilter::FilterBox::dropEvent(QDropEvent* event)
{
    const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*>(event->mimeData());
    if (!mime) // May happen when non-records (e.g. plain text) are dragged and dropped
        return;

    std::vector<CSMWorld::UniversalId> universalIdData = mime->getData();
    QModelIndex index = mime->getIndexAtDragStart();
    const CSVWorld::DragRecordTable* dragTable = mime->getTableOfDragStart();

    QVariant qData;
    std::string searchColumn;
    if (index.isValid() && dragTable)
    {
        qData = dragTable->model()->data(index);
        searchColumn = dragTable->model()->headerData(index.column(), Qt::Horizontal).toString().toStdString();
    }

    emit recordDropped(universalIdData, std::make_pair(qData, searchColumn), event->proposedAction());
}

void CSVFilter::FilterBox::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void CSVFilter::FilterBox::dragMoveEvent(QDragMoveEvent* event)
{
    event->accept();
}

void CSVFilter::FilterBox::createFilterRequest(const std::vector<FilterData>& sourceFilter, Qt::DropAction action)
{
    mRecordFilterBox->createFilterRequest(sourceFilter, action);
}
