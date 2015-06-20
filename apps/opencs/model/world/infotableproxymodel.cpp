#include "infotableproxymodel.hpp"

#include <components/misc/stringops.hpp>

#include "idtablebase.hpp"
#include "columns.hpp"

namespace
{
    QString toLower(const QString &str)
    {
        return Misc::StringUtils::lowerCase(str.toStdString()).c_str();
    }
}

CSMWorld::InfoTableProxyModel::InfoTableProxyModel(CSMWorld::UniversalId::Type type, QObject *parent)
    : IdTableProxyModel(parent),
      mType(type),
      mSourceModel(NULL)
{
    Q_ASSERT(type == UniversalId::Type_TopicInfos || type == UniversalId::Type_JournalInfos);
}

void CSMWorld::InfoTableProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    IdTableProxyModel::setSourceModel(sourceModel);
    mSourceModel = dynamic_cast<IdTableBase *>(sourceModel);
    if (mSourceModel != NULL)
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
    return IdTableProxyModel::lessThan(first, second);
}

int CSMWorld::InfoTableProxyModel::getFirstInfoRow(int currentRow) const
{
    Columns::ColumnId columnId = Columns::ColumnId_Topic;
    if (mType == UniversalId::Type_JournalInfos)
    {
        columnId = Columns::ColumnId_Journal;
    }

    int column = mSourceModel->findColumnIndex(columnId);
    QString info = toLower(mSourceModel->data(mSourceModel->index(currentRow, column)).toString());

    if (mFirstRowCache.contains(info))
    {
        return mFirstRowCache[info];
    }

    while (--currentRow >= 0 &&
           toLower(mSourceModel->data(mSourceModel->index(currentRow, column)).toString()) == info);
    ++currentRow;

    mFirstRowCache[info] = currentRow;
    return currentRow;
}

void CSMWorld::InfoTableProxyModel::modelRowsChanged(const QModelIndex &/*parent*/, int /*start*/, int /*end*/)
{
    mFirstRowCache.clear();
}
