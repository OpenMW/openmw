#ifndef CSV_FILTER_FILTERBOX_H
#define CSV_FILTER_FILTERBOX_H

#include <vector>

#include <QWidget>
#include <QtCore/qnamespace.h>

#include "../../model/filter/node.hpp"
#include "../../model/world/universalid.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSVFilter
{
    class RecordFilterBox;

    class FilterBox : public QWidget
    {
            Q_OBJECT

            RecordFilterBox *mRecordFilterBox;

        public:
            FilterBox (CSMWorld::Data& data, QWidget *parent = nullptr);

            void setRecordFilter (const std::string& filter);

            void createFilterRequest(std::vector<std::pair<std::string, std::vector<std::string> > >& filterSource,
                                     Qt::DropAction action);


        private:
            void dragEnterEvent (QDragEnterEvent* event) override;

            void dropEvent (QDropEvent* event) override;

            void dragMoveEvent(QDragMoveEvent *event) override;

        signals:
            void recordFilterChanged (std::shared_ptr<CSMFilter::Node> filter);
            void recordDropped (std::vector<CSMWorld::UniversalId>& types, Qt::DropAction action);
    };

}

#endif

