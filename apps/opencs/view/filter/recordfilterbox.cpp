
#include "recordfilterbox.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>

CSVFilter::RecordFilterBox::RecordFilterBox (QWidget *parent)
: QWidget (parent)
{
    QHBoxLayout *layout = new QHBoxLayout (this);

    layout->setContentsMargins (0, 0, 0, 0);

    layout->addWidget (new QLabel ("Record Filter", this));

    layout->addWidget (new QLineEdit (this));

    setLayout (layout);
}