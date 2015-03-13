
#include "reporttable.hpp"

#include <algorithm>

#include <QHeaderView>
#include <QAction>
#include <QMenu>

#include "../../model/tools/reportmodel.hpp"

#include "../../view/world/idtypedelegate.hpp"

void CSVTools::ReportTable::contextMenuEvent (QContextMenuEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    // create context menu
    QMenu menu (this);

    if (!selectedRows.empty())
    {
        menu.addAction (mShowAction);
        menu.addAction (mRemoveAction);
    }

    menu.exec (event->globalPos());
}

void CSVTools::ReportTable::mouseMoveEvent (QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        startDrag (*this);
}

void CSVTools::ReportTable::mouseDoubleClickEvent (QMouseEvent *event)
{
    Qt::KeyboardModifiers modifiers =
        event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

    QModelIndex index = currentIndex();

    selectionModel()->select (index,
        QItemSelectionModel::Clear | QItemSelectionModel::Select | QItemSelectionModel::Rows);

    switch (modifiers)
    {
        case 0:

            event->accept();
            showSelection();
            break;

        case Qt::ShiftModifier:

            event->accept();
            removeSelection();
            break;

        case Qt::ControlModifier:

            event->accept();
            showSelection();
            removeSelection();
            break;
    }
}

CSVTools::ReportTable::ReportTable (CSMDoc::Document& document,
    const CSMWorld::UniversalId& id, QWidget *parent)
: CSVWorld::DragRecordTable (document, parent), mModel (document.getReport (id))
{
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    verticalHeader()->hide();
    setSortingEnabled (true);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    setModel (mModel);
    setColumnHidden (2, true);

    mIdTypeDelegate = CSVWorld::IdTypeDelegateFactory().makeDelegate (
        document, this);

    setItemDelegateForColumn (0, mIdTypeDelegate);

    mShowAction = new QAction (tr ("Show"), this);
    connect (mShowAction, SIGNAL (triggered()), this, SLOT (showSelection()));
    addAction (mShowAction);

    mRemoveAction = new QAction (tr ("Remove from list"), this);
    connect (mRemoveAction, SIGNAL (triggered()), this, SLOT (removeSelection()));
    addAction (mRemoveAction);
}

std::vector<CSMWorld::UniversalId> CSVTools::ReportTable::getDraggedRecords() const
{
    std::vector<CSMWorld::UniversalId> ids;

    QModelIndexList selectedRows = selectionModel()->selectedRows();

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
    {
        ids.push_back (mModel->getUniversalId (iter->row()));
    }

    return ids;
}

void CSVTools::ReportTable::updateUserSetting (const QString& name, const QStringList& list)
{
    mIdTypeDelegate->updateUserSetting (name, list);
}

void CSVTools::ReportTable::showSelection()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
        emit editRequest (mModel->getUniversalId (iter->row()), mModel->getHint (iter->row()));
}

void CSVTools::ReportTable::removeSelection()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    std::reverse (selectedRows.begin(), selectedRows.end());

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
        mModel->removeRows (iter->row(), 1);

    selectionModel()->clear();
}
