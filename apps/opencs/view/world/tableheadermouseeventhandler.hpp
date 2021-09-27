#pragma once

#include <QHeaderView>
#include <QtGui>

namespace CSVWorld
{
class DragRecordTable;

class TableHeaderMouseEventHandler : public QWidget
{
public:
    explicit TableHeaderMouseEventHandler(DragRecordTable * parent);

    void showContextMenu(const QPoint &);

private:
    DragRecordTable & table;
    QHeaderView & header;

    QMenu & createContextMenu();
    bool eventFilter(QObject *, QEvent *) override;

}; // class TableHeaderMouseEventHandler
} // namespace CSVWorld
