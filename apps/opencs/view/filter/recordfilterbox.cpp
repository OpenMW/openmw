#include "recordfilterbox.hpp"

#include <variant>

#include <QHBoxLayout>
#include <QLabel>
#include <QVariant>

#include "editwidget.hpp"
#include "filterdata.hpp"

CSVFilter::RecordFilterBox::RecordFilterBox(CSMWorld::Data& worldData, QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 6, 5, 0);

    QLabel* label = new QLabel("Record Filter", this);
    label->setIndent(2);
    layout->addWidget(label);

    mEdit = new EditWidget(worldData, this);

    layout->addWidget(mEdit);

    setLayout(layout);

    connect(mEdit, &EditWidget::filterChanged, this, &RecordFilterBox::filterChanged);
}

void CSVFilter::RecordFilterBox::setFilter(const std::string& filter)
{
    mEdit->clear();
    mEdit->setText(QString::fromUtf8(filter.c_str()));
}

void CSVFilter::RecordFilterBox::createFilterRequest(const std::vector<FilterData>& sourceFilter, Qt::DropAction action)
{
    mEdit->createFilterRequest(sourceFilter, action);
}
