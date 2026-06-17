#ifndef CSV_FILTER_EDITWIDGET_H
#define CSV_FILTER_EDITWIDGET_H

#include <QLineEdit>
#include <QPalette>
#include <QVariant>

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "filterdata.hpp"

#include "../../model/filter/parser.hpp"

class QModelIndex;
class QAction;
class QContextMenuEvent;
class QObject;
class QWidget;

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
    enum class FilterType
    {
        String,
        Value
    };

    struct FilterVisitor
    {
        std::pair<std::string, FilterType> operator()(const std::string& stringData)
        {
            FilterType filterType = FilterType::String;
            return std::make_pair(stringData, filterType);
        }

        std::pair<std::string, FilterType> operator()(const QVariant& variantData)
        {
            FilterType filterType = FilterType::String;
            QMetaType::Type dataType = static_cast<QMetaType::Type>(variantData.typeId());
            if (dataType == QMetaType::QString || dataType == QMetaType::Bool || dataType == QMetaType::Int)
                filterType = FilterType::String;
            if (dataType == QMetaType::Int || dataType == QMetaType::Float)
                filterType = FilterType::Value;
            return std::make_pair(variantData.toString().toStdString(), filterType);
        }
    };

    class EditWidget : public QLineEdit
    {
        Q_OBJECT

        CSMFilter::Parser mParser;
        QPalette mPalette;
        bool mIsEmpty;
        int mStateColumnIndex;
        int mDescColumnIndex;
        QAction* mHelpAction;

    public:
        explicit EditWidget(CSMWorld::Data& worldData, QWidget* parent = nullptr);

        void createFilterRequest(const std::vector<FilterData>& sourceFilter, Qt::DropAction action);

    signals:

        void filterChanged(std::shared_ptr<CSMFilter::Node> filter);

    private:
        std::string generateFilter(const FilterData& filterData, FilterType filterType) const;

        void contextMenuEvent(QContextMenuEvent* event) override;

        constexpr std::string_view filterTypeName(const FilterType& type) const
        {
            switch (type)
            {
                case FilterType::String:
                    return "string";
                case FilterType::Value:
                    return "value";
            }
            return "unknown type";
        }

    private slots:

        void textChanged(const QString& text);

        void filterDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        void filterRowsRemoved(const QModelIndex& parent, int start, int end);

        void filterRowsInserted(const QModelIndex& parent, int start, int end);

        static void openHelp();
    };
}

#endif
