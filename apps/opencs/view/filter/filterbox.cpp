
#include "filterbox.hpp"

#include <QHBoxLayout>
#include <QDragEnterEvent>

#include "recordfilterbox.hpp"

#include <apps/opencs/model/world/tablemimedata.hpp>

CSVFilter::FilterBox::FilterBox (CSMWorld::Data& data, QWidget *parent)
: QWidget (parent)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    RecordFilterBox *recordFilterBox = new RecordFilterBox (data, this);

    layout->addWidget (recordFilterBox);

    setLayout (layout);

    connect (recordFilterBox,
        SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>)),
        this, SIGNAL (recordFilterChanged (boost::shared_ptr<CSMFilter::Node>)));

    connect(this, SIGNAL(createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >&, Qt::DropAction)),
            recordFilterBox, SIGNAL(createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >&, Qt::DropAction)));

    connect(this, SIGNAL(useFilterRequest(const std::string&)), recordFilterBox, SIGNAL(useFilterRequest(const std::string&)));
    setAcceptDrops(true);
}

void CSVFilter::FilterBox::dropEvent (QDropEvent* event)
{
    std::vector<CSMWorld::UniversalId> data = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData())->getData();

    emit recordDropped(data, event->proposedAction());
}

void CSVFilter::FilterBox::dragEnterEvent (QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void CSVFilter::FilterBox::dragMoveEvent (QDragMoveEvent* event)
{
    event->accept();
}
