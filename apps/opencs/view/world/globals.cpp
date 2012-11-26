
#include "globals.hpp"

#include <QTableView>
#include <QHeaderView>
#include <QSortFilterProxyModel>

#include "../../model/world/data.hpp"

CSVWorld::Globals::Globals (const CSMWorld::UniversalId& id, CSMWorld::Data& data)
: SubView (id)
{
    QTableView *table = new QTableView();

    setWidget (table);

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel (this);
    proxyModel->setSourceModel (data.getTableModel (id));

    table->setModel (proxyModel);
    table->horizontalHeader()->setResizeMode (QHeaderView::Interactive);
    table->verticalHeader()->hide();
    table->setSortingEnabled (true);

    /// \todo make initial layout fill the whole width of the table
}