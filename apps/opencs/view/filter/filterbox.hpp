#ifndef CSV_FILTER_FILTERBOX_H
#define CSV_FILTER_FILTERBOX_H

#include <QWidget>

#include "../../model/filter/node.hpp"

namespace CSVFilter
{
    class FilterBox : public QWidget
    {
            Q_OBJECT

        public:

            FilterBox (QWidget *parent = 0);

        signals:

            void recordFilterChanged (boost::shared_ptr<CSMFilter::Node> filter,
                const std::string& userValue);
    };

}

#endif
