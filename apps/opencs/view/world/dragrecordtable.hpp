#ifndef CSV_WORLD_DRAGRECORDTABLE_H
#define CSV_WORLD_DRAGRECORDTABLE_H

#include <QTableView>
#include <QEvent>

#include "../../model/world/columnbase.hpp"

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
        Q_OBJECT

        protected:
            CSMDoc::Document& mDocument;
            bool mEditLock;

        public:
            DragRecordTable(CSMDoc::Document& document, QWidget* parent = nullptr);

            virtual std::vector<CSMWorld::UniversalId> getDraggedRecords() const = 0;

            void setEditLock(bool locked);

        protected:
            void startDragFromTable(const DragRecordTable& table);

            void dragEnterEvent(QDragEnterEvent *event) override;

            void dragMoveEvent(QDragMoveEvent *event) override;

            void dropEvent(QDropEvent *event) override;

        private:
            CSMWorld::ColumnBase::Display getIndexDisplayType(const QModelIndex &index) const;

        signals:
            void moveRecordsFromSameTable(QDropEvent *event);
    };
}

#endif

