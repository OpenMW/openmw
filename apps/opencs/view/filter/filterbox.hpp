#ifndef CSV_FILTER_FILTERBOX_H
#define CSV_FILTER_FILTERBOX_H

#include <QWidget>

#include "../../model/filter/node.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSVFilter
{
    class FilterBox : public QWidget
    {
            Q_OBJECT

        public:

            FilterBox (CSMWorld::Data& data, QWidget *parent = 0);

        signals:

            void recordFilterChanged (boost::shared_ptr<CSMFilter::Node> filter);
    };

}

#endif
