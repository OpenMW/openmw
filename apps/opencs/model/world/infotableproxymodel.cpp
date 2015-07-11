#include "infotableproxymodel.hpp"

#include <components/misc/stringops.hpp>

#include "idtablebase.hpp"
#include "columns.hpp"

namespace
{
    QString toLower(const QString &str)
    {
        return QString::fromUtf8(Misc::StringUtils::lowerCase(str.toStdString()).c_str());
    }
}

CSMWorld::InfoTableProxyModel::InfoTableProxyModel(CSMWorld::UniversalId::Type type, QObject *parent)
    : IdTableProxyModel(parent),
      mType(type),
      mSourceModel(nullptr),
	  mInfoColumnId(type == UniversalId::Type_TopicInfos ? Columns::ColumnId_Topic : 
                                                           Columns::ColumnId_Journal)
{
    Q_ASSERT(type == UniversalId::Type_TopicInfos || type == UniversalId::Type_JournalInfos);
}

void CSMWorld::InfoTableProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    IdTableProxyModel::setSourceModel(sourceModel);
    mSourceModel = dynamic_cast<IdTableBase *>(sourceModel);
    if (mSourceModel != nullptr)
    {
        connect(mSourceModel, 
                SIGNAL(rowsInserted(const QModelIndex &, int, int)),
                this, 
                SLOT(modelRowsChanged(const QModelIndex &, int, int)));
        connect(mSourceModel, 
                SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
                this, 
                SLOT(modelRowsChanged(const QModelIndex &, int, int)));
        mFirstRowCache.clear();
    }
}

bool CSMWorld::InfoTableProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
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
    int row = currentRow;
    int column = mSourceModel->findColumnIndex(mInfoColumnId);
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

void CSMWorld::InfoTableProxyModel::modelRowsChanged(const QModelIndex &/*parent*/, int /*start*/, int /*end*/)
{
    mFirstRowCache.clear();
}
