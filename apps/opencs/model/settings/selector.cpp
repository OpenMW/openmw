#include <QItemSelectionRange>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QAbstractItemModel>

#include "selector.hpp"
#include "../../model/settings/setting.hpp"

#include <QDebug>

CSMSettings::Selector::Selector(const QString &name, const QStringList &list,
                                                            QObject *parent) :
    mProxyIndex (-1), mSelectionModel (0), mSelectAll (false),
    mDataModel (0), mProxySelectionModel (0), QObject(parent)
{
    setModel (new QStandardItemModel (this));
    addModelColumn (list);
    setObjectName (name);
}

void CSMSettings::Selector::addModelColumn (const QStringList &list)
{
    mDataModel->appendColumn (toStandardItemList (list));
}

void CSMSettings::Selector::addModelRow()
{
    QString empty = "";
    QStringList emptyRow (QStringList() << empty);

    for (int i = 1; i < columnCount(); i++)
        emptyRow.append(empty);

    mDataModel->appendRow (toStandardItemList (emptyRow));
}

int CSMSettings::Selector::columnCount() const
{
    return mDataModel->columnCount();
}

QModelIndexList CSMSettings::Selector::columnIndices (int column) const
{
    QModelIndexList indices;

    for (int i = 0; i < rowCount(); i++)
        indices.append (index (i, column));

    return indices;
}
QStringList CSMSettings::Selector::data (int row, int column) const
{
    return data (index (row, column));
}

QStringList CSMSettings::Selector::data (QModelIndex index) const
{
    if (!index.isValid())
        return QStringList();

    return mDataModel->data (index, Qt::UserRole + 1).toStringList();
}

void CSMSettings::Selector::dumpDataModel() const
{
    for (int i = 0; i < rowCount(); i++)
        for (int j = 0; j < columnCount(); j++)
            qDebug() << i << ',' << j << ':' << data (index (i, j));
}

QModelIndex CSMSettings::Selector::index (int row, int column) const
{
    if (!validCoordinate (row, column))
        return QModelIndex();

    return mDataModel->index (row, column);
}

bool CSMSettings::Selector::isSelected (const QModelIndex &idx) const
{
    return mSelectionModel->isSelected (idx);
}

QModelIndexList CSMSettings::Selector::matchByColumn
                            (const QStringList &matchValues, int column) const
{
    //matchByColumn searches the specified column for matches against the passed
    //value.  It returns a QModelIndexList of the indices which contain a match.
    qDebug() << objectName() <<  "Selector::matchByColumn() matching " << matchValues << " to column " << column;

    if (!validCoordinate (-1, column))
        return QModelIndexList();

    QModelIndexList matches;

    //If it's the first column, each index only contains one value.
    //we want to match all values in the column at once against the passed list
    if (column == 0) {
        QStringList columnValues = toStringList (matches);

        if (!stringListsMatch(columnValues, matchValues))
            matches = columnIndices(0);

        return matches;
    }

    //For columns greater than 0, we want to match values against each
    //stringlist in each index in the column.
    foreach (const QModelIndex &idx, columnIndices(column))
    {
        qDebug () << objectName() << "Selector::matchByColumn() value" << data (idx);
        if (stringListsMatch (data(idx), matchValues))
            matches.append (idx);
    }
    return matches;
}

void CSMSettings::Selector::refresh()
{
    qDebug() << "Selector::refresh()";
    QModelIndexList columnIndices = mSelectionModel->selectedIndexes();

    //for multi-column models (proxy masters), need to refresh the zero column
    //last to ensure the view updates correectly.
    if (columnCount() > 1)
    {
        QModelIndexList firstColumnIndices;
        QModelIndexList otherColumnIndices;

        foreach (const QModelIndex &idx, columnIndices)
        {
            if (idx.column() > 0)
                otherColumnIndices.append (idx);
            else
                firstColumnIndices.append (idx);
        }
        qDebug() << "Selector::refresh() Deselect / Select outer columns in master = " << otherColumnIndices.size();
        setSelection (otherColumnIndices, QItemSelectionModel::Clear, false);
        setSelection (otherColumnIndices, QItemSelectionModel::Select, false);

        qDebug() << "Selector::refresh() Deselect / Select first in master = " << firstColumnIndices.size();
        setSelection (otherColumnIndices, QItemSelectionModel::Clear, false);
        setSelection (otherColumnIndices, QItemSelectionModel::Select, true);
        return;
    }
qDebug() << "Selector::refresh() Deselect / Select column in slave = " << columnIndices.size();
    setSelection (columnIndices, QItemSelectionModel::Clear, false);
    setSelection (columnIndices, QItemSelectionModel::Select, true);

    if (mProxyIndex > 0)
    {
        QStandardItem *item = new QStandardItem();
        item->setData (toStringList (columnIndices));
        StandardItemList itemList;
        itemList.append (item);
        emit proxyUpdate (mProxyIndex, itemList);
    }
}

int CSMSettings::Selector::rowCount() const
{
    return mDataModel->rowCount();
}

QModelIndexList CSMSettings::Selector::rowIndices (int row) const
{
    QModelIndexList indices;

    for (int i = 0; i < columnCount(); i++)
        indices.append (index (row, i));

    return indices;
}

void CSMSettings::Selector::select (const QModelIndexList &list,
                                    QItemSelectionModel::SelectionFlags flags,
                                    bool emitUpdate) const
{
    qDebug () << objectName() << "::select (QModelIndexList) " << toStringList(list) << "; emit update? " << emitUpdate;
    QItemSelection selection;

    foreach (const QModelIndex &idx, list)
    {
        QItemSelectionRange idxRange (idx, idx);
        selection.append(idxRange);
    }

    select (selection, flags, emitUpdate);

}

void CSMSettings::Selector::select (const QItemSelection &selection,
                                QItemSelectionModel::SelectionFlags flags,
                                bool emitUpdate) const
{
    qDebug () << objectName() << "::select (QItemSelection) " << toStringList(selection.indexes()) << "; emit update? " << emitUpdate;
    mSelectionModel->select (selection, flags);

    if (emitUpdate)
        emit modelUpdate (toStringList (mSelectionModel->selectedIndexes()));
}
/*
void CSMSettings::Selector::select (const QModelIndex &idx,
                                QItemSelectionModel::SelectionFlags flags,
                                bool emitUpdate) const
{
    qDebug () << objectName() << "::select (QModelIndex) " << data(idx) << "; emit update? " << emitUpdate;

    mSelectionModel->select (idx, flags);

    if (emitUpdate)
        emit modelUpdate (toStringList (mSelectionModel->selectedIndexes()));
}
*/
void CSMSettings::Selector::selectAll() const
{
    qDebug () << "Selector::selectAll()";
    mSelectionModel->clear();

    select (QItemSelection (index(), index (rowCount() - 1)),
            QItemSelectionModel::Select);
}

QList <int> CSMSettings::Selector::selectedRows() const
{
    QList <int> rows;

    foreach (QModelIndex index, mSelectionModel->selectedRows())
        rows.append (index.row());

    return rows;
}

QItemSelection CSMSettings::Selector::selection() const
{
    return mSelectionModel->selection();
}

void CSMSettings::Selector::setData (const QStringList &list)
{
    qDebug () << "Selector::setData() values : " << list;

    if (mDataModel)
        mDataModel->clear();

    addModelRow();
    qDebug() << "model rows = " << rowCount() << "; cols = " << columnCount();

    mDataModel->setData (index(), list, Qt::UserRole + 1);
}

void CSMSettings::Selector::setData (int row, int column,
                                     const QStringList &valueList)
{

    QModelIndex idx = index (row, column);

    if (!idx.isValid())
        return;

    mDataModel->setData (idx, valueList, Qt::UserRole + 1);
}

void CSMSettings::Selector::setModel (QStandardItemModel *model)
{
    if (mDataModel)
        mDataModel->deleteLater();

    if (mSelectionModel)
        mSelectionModel->deleteLater();

    mDataModel = model;
    mSelectionModel = new QItemSelectionModel(model, this);

    connect (mSelectionModel,
             SIGNAL (selectionChanged (const QItemSelection &,
                                       const QItemSelection &)),
             this,
             SLOT (slotUpdate (const QItemSelection &,
                                    const QItemSelection &)));
}

void CSMSettings::Selector::setSelectAll ()
{
    mSelectAll = true;
}

void CSMSettings::Selector::setSelection (const QModelIndexList &indices,
                                    QItemSelectionModel::SelectionFlags flags,
                                          bool emitUpdate) const
{
    QItemSelection selection;
    qDebug() << "creating selection for indices " << toStringList (indices);

    foreach (const QModelIndex &idx, indices)
        selection.append(QItemSelectionRange(idx, idx));

    qDebug() << "Selector::setSelection(indices, flags) selecting " << selection.indexes().size() << " indices..." << " with flags " << flags;
    select (selection, flags, emitUpdate);
}

void CSMSettings::Selector::setViewSelection (const QStringList &list) const
{
    //Selects specific items in a column
    qDebug() << objectName() << "setViewSelection() values = " << list;

    QModelIndexList indices = columnIndices (0);
    QItemSelection selection;

    qDebug () << "\ttesting against " << toStringList (indices) << " = " << indices.size();

    //select only values which are already present in the selector's model
    foreach (QModelIndex idx, indices)
    {
        qDebug() << "\ttesting value " << data(idx);

        if (!list.contains (data (idx).at(0)))
            continue;

        qDebug () << "\tadding " << data(idx) << " to selection";
        QItemSelectionRange idxRange (idx, idx);
        selection.append (idxRange);
    }

    mSelectionModel->clear();
    qDebug() << objectName() << "::setViewSelection() selecting..." << selection.size();

    select (selection, QItemSelectionModel::Select, false);

    QStringList selectedValues = toStringList (selection.indexes());

    QStandardItem *item = new QStandardItem();

    item->setData (selectedValues);
    StandardItemList itemList;
    itemList.append (item);

    emit proxyUpdate (mProxyIndex, itemList);
}

void CSMSettings::Selector::slotUpdate (const QItemSelection &selected,
                                        const QItemSelection &deselected)
{/*
    qDebug () << objectName() << "::slotUpdate() selected = " << toStringList (selected.indexes());
    qDebug () << objectName() << "::slotUpdate() deselected = " << toStringList (deselected.indexes());
    QStringList selectedIndices = toStringList (selected.indexes());

    if (selectedIndices.size() == 0)
        return;

    //no proxy case
    if (mProxyIndex < 0)
        return;

    //slave proxy case
    if (mProxyIndex > 0)
    {
        StandardItemList list;
        QStandardItem *item = new QStandardItem;
        item->setData (selectedIndices);
        list.append (item);
        emit proxyUpdate (mProxyIndex, list);
        return;
    }
*/
    /*
    *   proxy index = 0 - master proxy case
    *
    *   emit a proxy update from the master only if the selection occured strictly in
    *   the first column and involded only one value (i.e. was a user selection)
    */
/*    qDebug() << objectName() << "::slotUpdate() master update";

    if (selectedIndices.isEmpty())
        return;

    if (selected.indexes().at(0).column() > 0)
        return;
     QModelIndexList rowList = rowIndices (selected.indexes().at(0).row());

     qDebug() << objectName() << "::slotUpdate() master update selecting row " << toStringList (rowList);

     //selecting the entire row is for the proxy selector only
     //model updates are not emitted from slotUpdate
     setSelection (rowList, QItemSelectionModel::Select, false);

     qDebug () << objectName() << "::slotUpdate() emit proxyUpdate, values = " << toStringList (rowList);

     emit proxyUpdate(mProxyIndex, toStandardItemList (rowList));

     qDebug() << objectName() << "::slotUpdate() complete";
     */
}

void CSMSettings::Selector::slotUpdateMasterByProxy (int column,
                                                     StandardItemList values)
{
    if (mProxyIndex != 0)
        return;

    if (values.isEmpty())
        return;

    //standarditemlist of values...  passed from proxy...  should be contained
    //entirely in the first element.
    QStringList valueStringList = values.at(0)->data().toStringList();

    if (valueStringList.isEmpty())
        return;

    QModelIndexList colMatchList = matchByColumn ( valueStringList, column);
    qDebug() << objectName() << "::slotUpdateMasterByProxy() column = " << column << "; values = " << valueStringList;

    setSelection (columnIndices (0), QItemSelectionModel::Deselect, false);
    setSelection (columnIndices (column), QItemSelectionModel::Deselect, false);

    //emit a blank list if no match found
    if (colMatchList.isEmpty())
    {
        emit modelUpdate (QStringList());
        return;
    }

    /*
     *Iterate all of the rows of the indexes in the colMatchList, looking for
     *one row which has the values of all of the selected indices as well.
     *
     *Failing that, select the match whose row has the most selections matched
     *Where not all selections are matched (except first column and the match column itself)
     */

    int winRow = -1;
    bool rowSelected = false;

    //iterate each of the matched indexes, retrieving rows to search
    foreach (const QModelIndex &idx, colMatchList)
    {
qDebug() << objectName() << "::slotUpdateMasterByProxy(); index = " << data (idx);

        QModelIndexList rowList = rowIndices (idx.row());

        //we always have a match, given that the index itself is a match
        int curRowCount = 1;
        int prevRowCount = 0;

        //iterate each index in the selected row, checking to see if it's
        //selected, apart from the current column and the first column
        foreach (const QModelIndex &idx2, rowList)
        {
qDebug() << objectName() << "::slotUpdateMasterByProxy(); rowlist index = " << data (idx2);

            int curCol = idx2.column();

            if ( (curCol == column) || (curCol == 0))
                continue;

            QStringList matchRowIndexList = data(idx2);

            //iterate each of the selections found in the selection model,
            //comparing stringlists only of the same column (setting).
            foreach (const QModelIndex &selIdx, mSelectionModel->selectedIndexes())
            {
                qDebug() << objectName() << "::slotUpdateMasterByProxy(); selIdx: " << data (selIdx);
                if (selIdx.column() == curCol)
                    if (stringListsMatch(data(selIdx), matchRowIndexList))
                        curRowCount++;
            }
        }

        /*
         *curRowCount could still be -1 if there are no current selections
         *in that case, the last matched column value always wins
         *
         *The entire row is selected if:
         *
         *      number of matches = number of columns - 1
         *      (exclude the first column)
         *
         *Otherwise, only select the winning index itself.
         */

        if (curRowCount >= prevRowCount)
        {
            winRow = idx.row();
            prevRowCount = curRowCount;
            rowSelected = (curRowCount == columnCount() - 1);
        }

        //if we've found a winning row, quit and select it
        if (rowSelected)
            break;
    }

    if (winRow == -1)
    {
        emit modelUpdate (QStringList());
        return;
    }

    /*
    * if an entire row is selected in the data model,
    * reflect that in the selection model.  Otherwise, select only the winning
    * index by row / column
    */
    qDebug () << objectName() << "slotUpdateMasterByProxy() winning row: " << data(winRow, 0) << "; row selected? " << rowSelected;

    qDebug () << objectName() << "selected index rows:";

    foreach (const QModelIndex &idx, mSelectionModel->selectedIndexes())
        qDebug() << idx.row() << ':' << data(idx);

    if (rowSelected) {
        QModelIndexList list;
        list.append (index(winRow));

        select(list, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    else
        select(QModelIndexList() << index(winRow, column), QItemSelectionModel::Select, false);
}

void CSMSettings::Selector::slotUpdateSlaveByProxy (int column,
                                                   StandardItemList values)
{
    qDebug () << objectName() << "::slotUpdateSlaveByProxy(); valueCount = " << values.size();

    //abort if this is not a slave proxy
    if (mProxyIndex < 1)
        return;

    QStringList valueStringList = values.at(mProxyIndex)->data().toStringList();

    //abort if the passed stringlist matches the data model
    if (stringListsMatch (toStringList (mSelectionModel->selectedIndexes()),
                          valueStringList))
    {
        return;
    }

    mSelectionModel->clear();

    qDebug () << objectName() << "::slotUpdateSlaveByProxy() received values = " << valueStringList;


    if (!mSelectAll)
    {
qDebug() << objectName() << "::slotUpdateSlaveByProxy() non-selectall...";
        setSelection (columnIndices(0), QItemSelectionModel::Deselect, false);

        QModelIndexList updatedSelections = matchByColumn (valueStringList);
        qDebug () << objectName() << "::slotUpdateSlaveByProxy() selecting " << updatedSelections;
        if (updatedSelections.isEmpty())
            return;

        setSelection (updatedSelections, QItemSelectionModel::Select);
        //refresh();
    }
    else
    {
qDebug() << objectName() << "::slotUpdateSlaveByProxy() selectall...";
        mDataModel->takeColumn (0);
        QStringList valueList = values.at(mProxyIndex)->data().toStringList();
        mDataModel->appendColumn (toStandardItemList(valueList));
        selectAll();
    }

    emit modelUpdate (valueStringList);
}

bool CSMSettings::Selector::stringListsMatch (const QStringList &list1,
                                              const QStringList &list2) const
{
    //success on exact match only
    if (list1.size() != list2.size())
        return false;

    for (int i =0; i < list1.size(); i++)
    {
        if (!(list2.at(i) == list1.at(i)))
            return false;
    }
    return true;
}

CSMSettings::StandardItemList CSMSettings::Selector::toStandardItemList
                                            (const QModelIndexList &list) const
{
    StandardItemList itemList;

    foreach (const QModelIndex &idx, list)
    {
        QStandardItem *item = new QStandardItem();
        item->setData (idx.data(Qt::UserRole + 1));
        itemList.append (item);
    }

    return itemList;
}

CSMSettings::StandardItemList CSMSettings::Selector::toStandardItemList
                                                (const QStringList &list) const
{
    StandardItemList itemList;

    foreach (const QString &value, list)
    {
        QStandardItem *item = new QStandardItem();
        item->setData (value);
        itemList.append (item);
    }

    return itemList;
}

QStringList CSMSettings::Selector::toStringList
                                       (const QModelIndexList &indices) const
{
    QStringList values;

    foreach (const QModelIndex &index, indices)
        values.append (data (index));

    return values;
}

bool CSMSettings::Selector::validCoordinate(int row, int column) const
{
    bool validRow = (row < 0);
    bool validColumn = (column < 0);

    if (!validRow)
        validRow = (row < rowCount());

    if (!validColumn)
        validColumn  = (column < columnCount());

    return validRow && validColumn;
}

