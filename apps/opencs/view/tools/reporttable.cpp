
#include "reporttable.hpp"

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
        menu.addAction (mShowAction);

    menu.exec (event->globalPos());
}

void CSVTools::ReportTable::mouseMoveEvent (QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        startDrag (*this);
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

    connect (this, SIGNAL (doubleClicked (const QModelIndex&)), this, SLOT (show (const QModelIndex&)));
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

void CSVTools::ReportTable::show (const QModelIndex& index)
{
    emit editRequest (mModel->getUniversalId (index.row()), mModel->getHint (index.row()));
}

void CSVTools::ReportTable::showSelection()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
        show (*iter);
}