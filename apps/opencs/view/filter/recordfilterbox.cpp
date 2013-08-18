
#include "recordfilterbox.hpp"

#include <QHBoxLayout>
#include <QLabel>

#include "editwidget.hpp"

CSVFilter::RecordFilterBox::RecordFilterBox (QWidget *parent)
: QWidget (parent)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    layout->addWidget (new QLabel ("Record Filter", this));

    EditWidget *editWidget = new EditWidget (this);

    layout->addWidget (editWidget);

    setLayout (layout);

    connect (
        editWidget, SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>, const std::string&)),
        this, SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>, const std::string&)));
}
