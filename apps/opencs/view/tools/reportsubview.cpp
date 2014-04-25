
#include "reportsubview.hpp"

#include <QTableView>
#include <QHeaderView>

#include "../../model/tools/reportmodel.hpp"

#include "../../view/world/idtypedelegate.hpp"

CSVTools::ReportSubView::ReportSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document)
: CSVDoc::SubView (id), mModel (document.getReport (id))
{
    setWidget (mTable = new QTableView (this));
    mTable->setModel (mModel);

    mTable->horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    mTable->verticalHeader()->hide();
    mTable->setSortingEnabled (true);
    mTable->setSelectionBehavior (QAbstractItemView::SelectRows);
    mTable->setSelectionMode (QAbstractItemView::ExtendedSelection);

    mIdTypeDelegate = CSVWorld::IdTypeDelegateFactory().makeDelegate (
        document.getUndoStack(), this);

    mTable->setItemDelegateForColumn (0, mIdTypeDelegate);

    connect (mTable, SIGNAL (doubleClicked (const QModelIndex&)), this, SLOT (show (const QModelIndex&)));
}

void CSVTools::ReportSubView::setEditLock (bool locked)
{
    // ignored. We don't change document state anyway.
}

void CSVTools::ReportSubView::updateUserSetting
                                (const QString &name, const QStringList &list)
{
    mIdTypeDelegate->updateUserSetting (name, list);
}

void CSVTools::ReportSubView::show (const QModelIndex& index)
{
    focusId (mModel->getUniversalId (index.row()), "");
}
