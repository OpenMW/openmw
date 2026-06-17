#include "tableheadermouseeventhandler.hpp"
#include "dragrecordtable.hpp"

#include <QAction>
#include <QEvent>
#include <QHeaderView>
#include <QMenu>
#include <QMouseEvent>
#include <QObject>
#include <QPoint>

namespace CSVWorld
{

    TableHeaderMouseEventHandler::TableHeaderMouseEventHandler(DragRecordTable* parent)
        : QWidget(parent)
        , table(*parent)
        , header(*table.horizontalHeader())
    {
        header.setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
        connect(&header, &QHeaderView::customContextMenuRequested,
            [this](const QPoint& position) { showContextMenu(position); });

        header.viewport()->installEventFilter(this);
    }

    bool TableHeaderMouseEventHandler::eventFilter(QObject* tableWatched, QEvent* event)
    {
        if (event->type() == QEvent::Type::MouseButtonPress)
        {
            auto& clickEvent = static_cast<QMouseEvent&>(*event);
            if ((clickEvent.button() == Qt::MiddleButton))
            {
                const auto& index = table.indexAt(clickEvent.position().toPoint());
                table.setColumnHidden(index.column(), true);
                clickEvent.accept();
                return true;
            }
        }
        return false;
    }

    void TableHeaderMouseEventHandler::showContextMenu(const QPoint& position)
    {
        auto& menu{ createContextMenu() };
        menu.popup(header.viewport()->mapToGlobal(position));
    }

    QMenu& TableHeaderMouseEventHandler::createContextMenu()
    {
        auto* menu = new QMenu(this);
        for (int i = 0; i < table.model()->columnCount(); ++i)
        {
            const auto& name = table.model()->headerData(i, Qt::Horizontal, Qt::DisplayRole);
            QAction* action{ new QAction(name.toString(), this) };
            action->setCheckable(true);
            action->setChecked(!table.isColumnHidden(i));
            menu->addAction(action);

            connect(action, &QAction::triggered, [this, action, i]() {
                table.setColumnHidden(i, !action->isChecked());
                action->setChecked(!action->isChecked());
                action->toggle();
            });
        }
        return *menu;
    }

} // namespace CSVWorld
