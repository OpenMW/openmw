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
    struct FilterVisitor
    {
        std::pair<std::string, std::string> operator()(const std::string& stringData)
        {
            std::string stringOrValue = "string";
            return std::make_pair(stringData, stringOrValue);
        }

        std::pair<std::string, std::string> operator()(const QVariant& variantData)
        {
            std::string stringOrValue;
            QMetaType::Type dataType = static_cast<QMetaType::Type>(variantData.type());
            if (dataType == QMetaType::QString || dataType == QMetaType::Bool || dataType == QMetaType::Int)
                stringOrValue = "string";
            if (dataType == QMetaType::Int || dataType == QMetaType::Float)
                stringOrValue = "value";
            return std::make_pair(variantData.toString().toStdString(), stringOrValue);
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
        EditWidget(CSMWorld::Data& data, QWidget* parent = nullptr);

        void createFilterRequest(
            std::vector<std::pair<std::variant<std::string, QVariant>, std::vector<std::string>>>& filterSource,
            Qt::DropAction action);

    signals:

        void filterChanged(std::shared_ptr<CSMFilter::Node> filter);

    private:
        std::string generateFilter(
            std::pair<std::string, std::vector<std::string>>& seekedString, std::string stringOrValue) const;

        void contextMenuEvent(QContextMenuEvent* event) override;

    private slots:

        void textChanged(const QString& text);

        void filterDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

        void filterRowsRemoved(const QModelIndex& parent, int start, int end);

        void filterRowsInserted(const QModelIndex& parent, int start, int end);

        static void openHelp();
    };
}

#endif
