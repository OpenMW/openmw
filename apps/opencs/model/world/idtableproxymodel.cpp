#include "idtableproxymodel.hpp"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QString>

#include <compare>
#include <type_traits>
#include <vector>

#include <apps/opencs/model/filter/node.hpp>
#include <apps/opencs/model/world/columns.hpp>

#include "columnbase.hpp"
#include "idtablebase.hpp"

class QObject;

namespace
{
    std::string getEnumValue(const std::vector<std::pair<int, std::string>>& values, int index)
    {
        if (index < 0 || index >= static_cast<int>(values.size()))
        {
            return "";
        }
        return values[index].second;
    }
}

void CSMWorld::IdTableProxyModel::updateColumnMap()
{
    Q_ASSERT(mSourceModel != nullptr);

    mColumnMap.clear();
    if (mFilter)
    {
        std::vector<int> columns = mFilter->getReferencedColumns();
        for (std::vector<int>::const_iterator iter(columns.begin()); iter != columns.end(); ++iter)
            mColumnMap.insert(std::make_pair(
                *iter, mSourceModel->searchColumnIndex(static_cast<CSMWorld::Columns::ColumnId>(*iter))));
    }
}

bool CSMWorld::IdTableProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    Q_ASSERT(mSourceModel != nullptr);

    // It is not possible to use filterAcceptsColumn() and check for
    // sourceModel()->headerData (sourceColumn, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags)
    // because the sourceColumn parameter excludes the hidden columns, i.e. wrong columns can
    // be rejected.  Workaround by disallowing tree branches (nested columns), which are not meant
    // to be visible, from the filter.
    if (sourceParent.isValid())
        return false;

    if (!mFilter)
        return true;

    return mFilter->test(*mSourceModel, sourceRow, mColumnMap);
}

CSMWorld::IdTableProxyModel::IdTableProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , mFilterTimer{ new QTimer(this) }
    , mSourceModel(nullptr)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);

    mFilterTimer->setSingleShot(true);
    int intervalSetting = CSMPrefs::State::get()["ID Tables"]["filter-delay"].toInt();
    mFilterTimer->setInterval(intervalSetting);

    connect(&CSMPrefs::State::get(), &CSMPrefs::State::settingChanged, this,
        [this](const CSMPrefs::Setting* setting) { this->settingChanged(setting); });
    connect(mFilterTimer.get(), &QTimer::timeout, this, [this]() { this->timerTimeout(); });
}

QModelIndex CSMWorld::IdTableProxyModel::getModelIndex(const std::string& id, int column) const
{
    Q_ASSERT(mSourceModel != nullptr);

    return mapFromSource(mSourceModel->getModelIndex(id, column));
}

void CSMWorld::IdTableProxyModel::setSourceModel(QAbstractItemModel* model)
{
    QSortFilterProxyModel::setSourceModel(model);

    mSourceModel = dynamic_cast<IdTableBase*>(sourceModel());
    connect(mSourceModel, &IdTableBase::rowsInserted, this, &IdTableProxyModel::sourceRowsInserted);
    connect(mSourceModel, &IdTableBase::rowsRemoved, this, &IdTableProxyModel::sourceRowsRemoved);
    connect(mSourceModel, &IdTableBase::dataChanged, this, &IdTableProxyModel::sourceDataChanged);
}

void CSMWorld::IdTableProxyModel::setFilter(const std::shared_ptr<CSMFilter::Node>& filter)
{
    mAwaitingFilter = filter;
    mFilterTimer->start();
}

bool CSMWorld::IdTableProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    Columns::ColumnId id = static_cast<Columns::ColumnId>(left.data(ColumnBase::Role_ColumnId).toInt());
    EnumColumnCache::const_iterator valuesIt = mEnumColumnCache.find(id);
    if (valuesIt == mEnumColumnCache.end())
    {
        if (Columns::hasEnums(id))
        {
            valuesIt = mEnumColumnCache.insert(std::make_pair(id, Columns::getEnums(id))).first;
        }
    }

    if (valuesIt != mEnumColumnCache.end())
    {
        std::string first = getEnumValue(valuesIt->second, left.data().toInt());
        std::string second = getEnumValue(valuesIt->second, right.data().toInt());
        return first < second;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

QString CSMWorld::IdTableProxyModel::getRecordId(int sourceRow) const
{
    Q_ASSERT(mSourceModel != nullptr);

    int idColumn = mSourceModel->findColumnIndex(Columns::ColumnId_Id);
    return mSourceModel->data(mSourceModel->index(sourceRow, idColumn)).toString();
}

void CSMWorld::IdTableProxyModel::refreshFilter()
{
    if (mFilter)
    {
        updateColumnMap();
        invalidateFilter();
    }
}

void CSMWorld::IdTableProxyModel::timerTimeout()
{
    if (mAwaitingFilter)
    {
        beginResetModel();
        mFilter = mAwaitingFilter;
        updateColumnMap();
        endResetModel();
        mAwaitingFilter.reset();
    }
}

void CSMWorld::IdTableProxyModel::settingChanged(const CSMPrefs::Setting* setting)
{
    if (*setting == "ID Tables/filter-delay")
    {
        mFilterTimer->setInterval(setting->toInt());
    }
}

void CSMWorld::IdTableProxyModel::sourceRowsInserted(const QModelIndex& parent, int /*start*/, int end)
{
    refreshFilter();
    if (!parent.isValid())
    {
        emit rowAdded(getRecordId(end).toUtf8().constData());
    }
}

void CSMWorld::IdTableProxyModel::sourceRowsRemoved(const QModelIndex& /*parent*/, int /*start*/, int /*end*/)
{
    refreshFilter();
}

void CSMWorld::IdTableProxyModel::sourceDataChanged(const QModelIndex& /*topLeft*/, const QModelIndex& /*bottomRight*/)
{
    refreshFilter();
}
