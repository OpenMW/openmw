
#include "filterbox.hpp"

#include <QHBoxLayout>
#include <QDragEnterEvent>

#include "recordfilterbox.hpp"

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

}

void CSVFilter::FilterBox::dragEnterEvent (QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void CSVFilter::FilterBox::dragMoveEvent (QDragMoveEvent* event)
{
    event->accept();
}