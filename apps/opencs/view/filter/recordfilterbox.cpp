
#include "recordfilterbox.hpp"

#include <QHBoxLayout>
#include <QLabel>

#include "editwidget.hpp"

CSVFilter::RecordFilterBox::RecordFilterBox (CSMWorld::Data& data, QWidget *parent)
: QWidget (parent)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    layout->addWidget (new QLabel ("Record Filter", this));

    mEdit = new EditWidget (data, this);

    layout->addWidget (mEdit);

    setLayout (layout);

    connect (
        mEdit, SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>)),
        this, SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>)));
}

void CSVFilter::RecordFilterBox::setFilter (const std::string& filter)
{
    mEdit->clear();
    mEdit->setText (QString::fromUtf8 (filter.c_str()));
}

void CSVFilter::RecordFilterBox::createFilterRequest (std::vector< std::pair< std::string, std::vector< std::string > > >& filterSource,
                                                      Qt::DropAction action)
{
    mEdit->createFilterRequest(filterSource, action);
}
