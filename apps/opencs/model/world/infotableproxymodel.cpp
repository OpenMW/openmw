#include "infotableproxymodel.hpp"

#include <components/misc/stringops.hpp>

#include "idtablebase.hpp"
#include "columns.hpp"

namespace
{
    QString toLower(const QString &str)
    {
        return QString::fromUtf8(Misc::StringUtils::lowerCase(str.toUtf8().constData()).c_str());
    }
}

CSMWorld::InfoTableProxyModel::InfoTableProxyModel(CSMWorld::UniversalId::Type type, QObject *parent)
    : IdTableProxyModel(parent),
      mType(type),
      mInfoColumnId(type == UniversalId::Type_TopicInfos ? Columns::ColumnId_Topic :
                                                           Columns::ColumnId_Journal),
      mInfoColumnIndex(-1),
      mLastAddedSourceRow(-1)
{
    Q_ASSERT(type == UniversalId::Type_TopicInfos || type == UniversalId::Type_JournalInfos);
}

void CSMWorld::InfoTableProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    IdTableProxyModel::setSourceModel(sourceModel);

    if (mSourceModel != nullptr)
    {
        mInfoColumnIndex = mSourceModel->findColumnIndex(mInfoColumnId);
        mFirstRowCache.clear();
    }
}

bool CSMWorld::InfoTableProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    Q_ASSERT(mSourceModel != nullptr);

    QModelIndex first = mSourceModel->index(getFirstInfoRow(left.row()), left.column());
    QModelIndex second = mSourceModel->index(getFirstInfoRow(right.row()), right.column());

    // If both indexes are belonged to the same Topic/Journal, compare their original rows only
    if (first.row() == second.row())
    {
        return sortOrder() == Qt::AscendingOrder ? left.row() < right.row() : right.row() < left.row();
    }
    return IdTableProxyModel::lessThan(first, second);
}

int CSMWorld::InfoTableProxyModel::getFirstInfoRow(int currentRow) const
{
    Q_ASSERT(mSourceModel != nullptr);

    int row = currentRow;
    int column = mInfoColumnIndex;
    QString info = toLower(mSourceModel->data(mSourceModel->index(row, column)).toString());

    if (mFirstRowCache.contains(info))
    {
        return mFirstRowCache[info];
    }

    while (--row >= 0 && 
           toLower(mSourceModel->data(mSourceModel->index(row, column)).toString()) == info);
    ++row;

    mFirstRowCache[info] = row;
    return row;
}

void CSMWorld::InfoTableProxyModel::sourceRowsRemoved(const QModelIndex &/*parent*/, int /*start*/, int /*end*/)
{
    refreshFilter();
    mFirstRowCache.clear();
}

void CSMWorld::InfoTableProxyModel::sourceRowsInserted(const QModelIndex &parent, int /*start*/, int end)
{
    refreshFilter();

    if (!parent.isValid())
    {
        mFirstRowCache.clear();
        // We can't re-sort the model here, because the topic of the added row isn't set yet.
        // Store the row index for using in the first dataChanged() after this row insertion.
        mLastAddedSourceRow = end;
    }
}

void CSMWorld::InfoTableProxyModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    refreshFilter();

    if (mLastAddedSourceRow != -1 && 
        topLeft.row() <= mLastAddedSourceRow && bottomRight.row() >= mLastAddedSourceRow)
    {
        // Now the topic of the last added row is set, 
        // so we can re-sort the model to ensure the corrent position of this row
        int column = sortColumn();
        Qt::SortOrder order = sortOrder();
        sort(mInfoColumnIndex); // Restore the correct position of an added row
        sort(column, order);    // Restore the original sort order
        emit rowAdded(getRecordId(mLastAddedSourceRow).toUtf8().constData());

        // Make sure that we perform a re-sorting only in the first dataChanged() after a row insertion
        mLastAddedSourceRow = -1;
    }
}
