#ifndef CSV_WORLD_TABLESUBVIEW_H
#define CSV_WORLD_TABLESUBVIEW_H

#include "../doc/subview.hpp"

#include <QtCore/qnamespace.h>

class QModelIndex;

namespace CSMWorld
{
    class IdTable;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVFilter
{
    class FilterBox;
}

namespace CSVWorld
{
    class Table;
    class TableBottomBox;
    class CreatorFactoryBase;

    class TableSubView : public CSVDoc::SubView
    {
            Q_OBJECT

            Table *mTable;
            TableBottomBox *mBottom;
            CSVFilter::FilterBox *mFilterBox;

        public:

            TableSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
                const CreatorFactoryBase& creatorFactory, bool sorting);

            void setEditLock (bool locked) override;

            void setStatusBar (bool show) override;

            void useHint (const std::string& hint) override;

        protected:
            bool eventFilter(QObject* object, QEvent *event) override;

        signals:
            void cloneRequest(const std::string&,
                              const CSMWorld::UniversalId::Type);

        private slots:

            void editRequest (const CSMWorld::UniversalId& id, const std::string& hint);
            void cloneRequest (const CSMWorld::UniversalId& toClone);
            void createFilterRequest(std::vector< CSMWorld::UniversalId >& types,
                                     Qt::DropAction action);

        public slots:

            void requestFocus (const std::string& id);
    };
}

#endif
