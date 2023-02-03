#ifndef CSV_FILTER_FILTERDATA_H
#define CSV_FILTER_FILTERDATA_H

#include <string>
#include <variant>
#include <vector>

#include <QVariant>

namespace CSVFilter
{
    struct FilterData
    {
        std::variant<std::string, QVariant> searchData;
        std::vector<std::string> columns;
    };
}

#endif
