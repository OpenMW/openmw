
#include "regionmapsubview.hpp"

#include <QTableView>
#include <QHeaderView>

CSVWorld::RegionMapSubView::RegionMapSubView (CSMWorld::UniversalId universalId,
    CSMDoc::Document& document)
: CSVDoc::SubView (universalId)
{
    mTable = new QTableView (this);

    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();

    mTable->setSelectionMode (QAbstractItemView::ExtendedSelection);

    mTable->setModel (document.getData().getTableModel (universalId));

    mTable->resizeColumnsToContents();
    mTable->resizeRowsToContents();

    setWidget (mTable);
}

void CSVWorld::RegionMapSubView::setEditLock (bool locked)
{

}

void CSVWorld::RegionMapSubView::updateUserSetting
                                 (const QString &sname, const QStringList &list)
{}
