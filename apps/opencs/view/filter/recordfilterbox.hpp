#ifndef CSV_FILTER_RECORDFILTERBOX_H
#define CSV_FILTER_RECORDFILTERBOX_H

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <QVariant>
#include <QWidget>

#include "filterdata.hpp"

namespace CSMFilter
{
    class Node;
}

namespace CSMWorld
{
    class Data;
}

namespace CSVFilter
{
    class EditWidget;

    class RecordFilterBox : public QWidget
    {
        Q_OBJECT

        EditWidget* mEdit;

    public:
        explicit RecordFilterBox(CSMWorld::Data& worldData, QWidget* parent = nullptr);

        void setFilter(const std::string& filter);

        void useFilterRequest(const std::string& idOfFilter);

        void createFilterRequest(const std::vector<FilterData>& sourceFilter, Qt::DropAction action);

    signals:

        void filterChanged(std::shared_ptr<CSMFilter::Node> filter);
    };

}

#endif
