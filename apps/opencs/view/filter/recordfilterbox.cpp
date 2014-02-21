
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

    EditWidget *editWidget = new EditWidget (data, this);

    layout->addWidget (editWidget);

    setLayout (layout);

    connect (
        editWidget, SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>)),
        this, SIGNAL (filterChanged (boost::shared_ptr<CSMFilter::Node>)));

    connect(this, SIGNAL(createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >&, Qt::DropAction)),
            editWidget, SLOT(createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >&, Qt::DropAction)));

    connect(this, SIGNAL(useFilterRequest(const std::string&)), editWidget, SLOT(useFilterRequest(const std::string&)));
}
