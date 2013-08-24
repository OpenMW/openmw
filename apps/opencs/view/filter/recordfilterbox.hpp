#ifndef CSV_FILTER_RECORDFILTERBOX_H
#define CSV_FILTER_RECORDFILTERBOX_H

#include <boost/shared_ptr.hpp>

#include <QWidget>

#include <QHBoxLayout>

#include "../../model/filter/node.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSVFilter
{
    class RecordFilterBox : public QWidget
    {
            Q_OBJECT

        public:

            RecordFilterBox (const CSMWorld::Data& data, QWidget *parent = 0);

        signals:

            void filterChanged (boost::shared_ptr<CSMFilter::Node> filter);
    };

}

#endif