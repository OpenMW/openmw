#ifndef CSV_WORLD_NESTEDTABLE_H
#define CSV_WORLD_NESTEDTABLE_H

#include <QEvent>

#include "dragrecordtable.hpp"

class QAction;
class QContextMenuEvent;

namespace CSMWorld
{
    class NestedTableProxyModel;
    class UniversalId;
    class CommandDispatcher;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class TableEditIdAction;

    class NestedTable : public DragRecordTable
    {
        Q_OBJECT

        QAction *mAddNewRowAction;
        QAction *mRemoveRowAction;
        TableEditIdAction *mEditIdAction;
        CSMWorld::NestedTableProxyModel* mModel;
        CSMWorld::CommandDispatcher *mDispatcher;

    public:
        NestedTable(CSMDoc::Document& document,
                    CSMWorld::UniversalId id,
                    CSMWorld::NestedTableProxyModel* model,
                    QWidget* parent = nullptr,
                    bool editable = true,
                    bool fixedRows = false);

        std::vector<CSMWorld::UniversalId> getDraggedRecords() const override;

    private:
        void contextMenuEvent (QContextMenuEvent *event) override;

    private slots:
        void removeRowActionTriggered();

        void addNewRowActionTriggered();

        void editCell();

    signals:
        void editRequest(const CSMWorld::UniversalId &id, const std::string &hint);
    };
}

#endif
