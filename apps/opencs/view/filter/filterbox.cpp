#include "filterbox.hpp"

#include <string>
#include <utility>
#include <vector>

#include <QDragEnterEvent>
#include <QHBoxLayout>

#include "recordfilterbox.hpp"

#include <components/debug/debuglog.hpp>

#include <apps/opencs/model/world/tablemimedata.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/world/dragrecordtable.hpp>

CSVFilter::FilterBox::FilterBox(CSMWorld::Data& data, QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);

    mRecordFilterBox = new RecordFilterBox(data, this);

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

    std::string searchString = "";
    if (index.isValid() && dragTable)
        searchString = dragTable->model()->data(index).toString().toStdString();
    Log(Debug::Warning) << "Data: " << searchString;

    std::string searchColumn = "";
    if (index.isValid() && dragTable)
        searchColumn = dragTable->model()->headerData(index.column(), Qt::Horizontal).toString().toStdString();
    Log(Debug::Warning) << "Header: " << searchColumn;

    emit recordDropped(universalIdData, event->proposedAction(), searchString, searchColumn);
}

void CSVFilter::FilterBox::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void CSVFilter::FilterBox::dragMoveEvent(QDragMoveEvent* event)
{
    event->accept();
}

void CSVFilter::FilterBox::createFilterRequest(
    std::vector<std::pair<std::string, std::vector<std::string>>>& filterSource, Qt::DropAction action)
{
    mRecordFilterBox->createFilterRequest(filterSource, action);
}
