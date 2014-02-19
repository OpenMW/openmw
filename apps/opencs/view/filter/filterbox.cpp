
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

    setAcceptDrops(true);
}

void CSVFilter::FilterBox::dropEvent (QDropEvent* event)
{
    std::vector<CSMWorld::UniversalId> data = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData())->getData();

    emit recordDropped(data);
}

void CSVFilter::FilterBox::dragEnterEvent (QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void CSVFilter::FilterBox::dragMoveEvent (QDragMoveEvent* event)
{
    event->accept();
}

void CSVFilter::FilterBox::createFilter (std::vector< std::pair< std::string, std::vector< std::string > > >& filterSource)
{
    for (unsigned i = 0; i < filterSource.size(); ++i) //test
    {
        std::cout<<filterSource[i].first<<std::endl;
        std::cout<<"Columns:\n";
        for (unsigned j = 0; j < filterSource[i].second.size(); ++j)
        {
            std::cout<<filterSource[i].second[j]<<std::endl;
        }
        std::cout<<"\n";
    }
}
