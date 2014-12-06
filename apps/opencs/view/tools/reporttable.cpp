
#include "reporttable.hpp"

#include <QHeaderView>

#include "../../model/tools/reportmodel.hpp"

#include "../../view/world/idtypedelegate.hpp"

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

    mIdTypeDelegate = CSVWorld::IdTypeDelegateFactory().makeDelegate (
        document, this);

    setItemDelegateForColumn (0, mIdTypeDelegate);

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
    emit editRequest (mModel->getUniversalId (index.row()), "");
}