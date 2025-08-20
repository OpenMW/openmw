#ifndef CSV_FILTER_FILTERBOX_H
#define CSV_FILTER_FILTERBOX_H

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <QVariant>
#include <QWidget>

#include "filterdata.hpp"

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QObject;

namespace CSMFilter
{
    class Node;
}
namespace CSMWorld
{
    class Data;
    class UniversalId;
}

namespace CSVFilter
{
    class RecordFilterBox;

    class FilterBox : public QWidget
    {
        Q_OBJECT

        RecordFilterBox* mRecordFilterBox;

    public:
        explicit FilterBox(CSMWorld::Data& worldData, QWidget* parent = nullptr);

        void setRecordFilter(const std::string& filter);

        void createFilterRequest(const std::vector<FilterData>& sourceFilter, Qt::DropAction action);

    private:
        void dragEnterEvent(QDragEnterEvent* event) override;

        void dropEvent(QDropEvent* event) override;

        void dragMoveEvent(QDragMoveEvent* event) override;

    signals:
        void recordFilterChanged(std::shared_ptr<CSMFilter::Node> filter);
        void recordDropped(std::vector<CSMWorld::UniversalId>& types,
            const std::pair<QVariant, std::string>& columnSearchData, Qt::DropAction action);
    };

}

#endif
