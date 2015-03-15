#ifndef CSV_WORLD_DRAGRECORDTABLE_H
#define CSV_WORLD_DRAGRECORDTABLE_H

#include <QTableView>
#include <QtGui/qevent.h>

class QWidget;
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
            bool mEditLock;

        public:
            DragRecordTable(CSMDoc::Document& document, QWidget* parent = NULL);

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const = 0;

            void setEditLock(bool locked);

        protected:
            void startDragFromTable(const DragRecordTable& table);

            void dragEnterEvent(QDragEnterEvent *event);

            void dragMoveEvent(QDragMoveEvent *event);
    };
}

#endif

