#ifndef CSV_FILTER_RECORDFILTERBOX_H
#define CSV_FILTER_RECORDFILTERBOX_H

#include <QWidget>
#include <QtCore/qnamespace.h>

#include <QHBoxLayout>

#include "../../model/filter/node.hpp"

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

            EditWidget *mEdit;

        public:

            RecordFilterBox (CSMWorld::Data& data, QWidget *parent = nullptr);

            void setFilter (const std::string& filter);

            void useFilterRequest(const std::string& idOfFilter);

            void createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >& filterSource,
                                     Qt::DropAction action);

        signals:

            void filterChanged (std::shared_ptr<CSMFilter::Node> filter);
    };

}

#endif
