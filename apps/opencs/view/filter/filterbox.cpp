
#include "filterbox.hpp"

#include <QHBoxLayout>

#include "recordfilterbox.hpp"

CSVFilter::FilterBox::FilterBox (QWidget *parent)
: QWidget (parent)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    layout->addWidget (new RecordFilterBox (this));

    setLayout (layout);
}