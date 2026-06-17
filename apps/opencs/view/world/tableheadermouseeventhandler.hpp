#ifndef OPENMW_APPS_OPENCS_VIEW_WORLD_TABLEHEADERMOUSEEVENTHANDLER_HPP
#define OPENMW_APPS_OPENCS_VIEW_WORLD_TABLEHEADERMOUSEEVENTHANDLER_HPP

#include <QWidget>

class QEvent;
class QHeaderView;
class QMenu;
class QObject;
class QPoint;

namespace CSVWorld
{
    class DragRecordTable;

    class TableHeaderMouseEventHandler : public QWidget
    {
    public:
        explicit TableHeaderMouseEventHandler(DragRecordTable* parent);

        void showContextMenu(const QPoint&);

    private:
        DragRecordTable& table;
        QHeaderView& header;

        QMenu& createContextMenu();
        bool eventFilter(QObject*, QEvent*) override;

    }; // class TableHeaderMouseEventHandler
} // namespace CSVWorld

#endif
