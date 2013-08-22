
#include "filterbox.hpp"

#include <QHBoxLayout>

#include "recordfilterbox.hpp"

CSVFilter::FilterBox::FilterBox (QWidget *parent)
: QWidget (parent)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    RecordFilterBox *recordFilterBox = new RecordFilterBox (this);

    layout->addWidget (recordFilterBox);

    setLayout (layout);

    connect (recordFilterBox,
        SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>)),
        this, SIGNAL (recordFilterChanged (boost::shared_ptr<CSMFilter::Node>)));
}