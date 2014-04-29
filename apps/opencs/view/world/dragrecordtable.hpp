#ifndef CSV_WORLD_REGIONMAP_H
#define CSV_WORLD_REGIONMAP_H

#include <QTableView>

class QAction;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class UniversalId;
}

namespace CSVWorld
{
    class DragRecordTable : public QTableView
    {
        protected:
            CSMDoc::Document& mDocument;

        public:
            DragRecordTable(CSMDoc::Document& document);

            virtual std::vector<CSMWorld::UniversalId> getDragedRecords() const = 0;

        protected:
            void startDrag(const DragRecordTable& table);
    };
}

#endif
 
