#include "idtable.hpp"

#include <QModelIndex>
#include <QVariant>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <stddef.h>
#include <stdexcept>
#include <type_traits>

#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/idtablebase.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadcell.hpp>
#include <components/misc/strings/lower.hpp>

#include "collectionbase.hpp"
#include "columnbase.hpp"

CSMWorld::IdTable::IdTable(CollectionBase* idCollection, unsigned int features)
    : IdTableBase(features)
    , mIdCollection(idCollection)
{
}

int CSMWorld::IdTable::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mIdCollection->getSize();
}

int CSMWorld::IdTable::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mIdCollection->getColumns();
}

QVariant CSMWorld::IdTable::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.column() < 0)
        return QVariant();

    if (role == ColumnBase::Role_Display)
        return QVariant(mIdCollection->getColumn(index.column()).mDisplayType);

    if (role == ColumnBase::Role_ColumnId)
        return QVariant(getColumnId(index.column()));

    if ((role != Qt::DisplayRole && role != Qt::EditRole))
        return QVariant();

    if (role == Qt::EditRole && !mIdCollection->getColumn(index.column()).isEditable())
        return QVariant();

    return mIdCollection->getData(index.row(), index.column());
}

QVariant CSMWorld::IdTable::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QVariant();

    if (orientation != Qt::Horizontal)
        throw std::logic_error("Unknown header orientation specified");

    if (role == Qt::DisplayRole)
        return tr(mIdCollection->getColumn(section).getTitle().c_str());

    if (role == ColumnBase::Role_Flags)
        return mIdCollection->getColumn(section).mFlags;

    if (role == ColumnBase::Role_Display)
        return mIdCollection->getColumn(section).mDisplayType;

    if (role == ColumnBase::Role_ColumnId)
        return getColumnId(section);

    return QVariant();
}

bool CSMWorld::IdTable::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (mIdCollection->getColumn(index.column()).isEditable() && role == Qt::EditRole)
    {
        mIdCollection->setData(index.row(), index.column(), value);

        int stateColumn = searchColumnIndex(Columns::ColumnId_Modification);
        if (stateColumn != -1)
        {
            if (index.column() == stateColumn)
            {
                // modifying the state column can modify other values. we need to tell
                // views that the whole row has changed.

                emit dataChanged(
                    this->index(index.row(), 0), this->index(index.row(), columnCount(index.parent()) - 1));
            }
            else
            {
                emit dataChanged(index, index);

                // Modifying a value can also change the Modified status of a record.
                QModelIndex stateIndex = this->index(index.row(), stateColumn);
                emit dataChanged(stateIndex, stateIndex);
            }
        }
        else
            emit dataChanged(index, index);

        return true;
    }

    return false;
}

Qt::ItemFlags CSMWorld::IdTable::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (mIdCollection->getColumn(index.column()).isUserEditable())
        flags |= Qt::ItemIsEditable;

    int blockedColumn = searchColumnIndex(Columns::ColumnId_Blocked);
    if (blockedColumn != -1 && blockedColumn != index.column())
    {
        bool isBlocked = mIdCollection->getData(index.row(), blockedColumn).toInt();
        if (isBlocked)
            flags = Qt::ItemIsSelectable; // not enabled (to grey out)
    }

    return flags;
}

bool CSMWorld::IdTable::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
        return false;

    beginRemoveRows(parent, row, row + count - 1);

    mIdCollection->removeRows(row, count);

    endRemoveRows();

    return true;
}

QModelIndex CSMWorld::IdTable::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid())
        return QModelIndex();

    if (row < 0 || row >= mIdCollection->getSize())
        return QModelIndex();

    if (column < 0 || column >= mIdCollection->getColumns())
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex CSMWorld::IdTable::parent(const QModelIndex& index) const
{
    return QModelIndex();
}

void CSMWorld::IdTable::addRecord(const std::string& id, UniversalId::Type type)
{
    ESM::RefId refId = ESM::RefId::stringRefId(id);
    int index = mIdCollection->getAppendIndex(refId, type);

    beginInsertRows(QModelIndex(), index, index);

    mIdCollection->appendBlankRecord(refId, type);

    endInsertRows();
}

void CSMWorld::IdTable::addRecordWithData(
    const std::string& id, const std::map<int, QVariant>& data, UniversalId::Type type)
{
    ESM::RefId refId = ESM::RefId::stringRefId(id);
    int index = mIdCollection->getAppendIndex(refId, type);

    beginInsertRows(QModelIndex(), index, index);

    mIdCollection->appendBlankRecord(refId, type);

    for (std::map<int, QVariant>::const_iterator iter(data.begin()); iter != data.end(); ++iter)
    {
        mIdCollection->setData(index, iter->first, iter->second);
    }

    endInsertRows();
}

void CSMWorld::IdTable::cloneRecord(
    const ESM::RefId& origin, const ESM::RefId& destination, CSMWorld::UniversalId::Type type)
{
    int index = mIdCollection->getAppendIndex(destination, type);

    beginInsertRows(QModelIndex(), index, index);
    mIdCollection->cloneRecord(origin, destination, type);
    endInsertRows();
}

bool CSMWorld::IdTable::touchRecord(const std::string& id)
{
    ESM::RefId refId = ESM::RefId::stringRefId(id);
    bool changed = mIdCollection->touchRecord(refId);

    int row = mIdCollection->getIndex(refId);
    int column = mIdCollection->searchColumnIndex(Columns::ColumnId_RecordType);
    if (changed && column != -1)
    {
        QModelIndex modelIndex = index(row, column);
        emit dataChanged(modelIndex, modelIndex);
    }

    return changed;
}

std::string CSMWorld::IdTable::getId(int row) const
{
    return mIdCollection->getId(row).getRefIdString();
}

/// This method can return only indexes to the top level table cells
QModelIndex CSMWorld::IdTable::getModelIndex(const std::string& id, int column) const
{
    const int row = mIdCollection->searchId(ESM::RefId::stringRefId(id));

    if (row != -1)
        return index(row, column);

    return QModelIndex();
}

void CSMWorld::IdTable::setRecord(
    const std::string& id, std::unique_ptr<RecordBase> record, CSMWorld::UniversalId::Type type)
{
    const ESM::RefId refId = ESM::RefId::stringRefId(id);
    const int index = mIdCollection->searchId(refId);

    if (index == -1)
    {
        // For info records, appendRecord may use a different index than the one returned by
        // getAppendIndex (because of prev/next links).  This can result in the display not
        // updating correctly after an undo
        //
        // Use an alternative method to get the correct index.  For non-Info records the
        // record pointer is ignored and internally calls getAppendIndex.
        const int index2 = mIdCollection->getInsertIndex(refId, type, record.get());

        beginInsertRows(QModelIndex(), index2, index2);

        mIdCollection->appendRecord(std::move(record), type);

        endInsertRows();
    }
    else
    {
        mIdCollection->replace(index, std::move(record));
        emit dataChanged(
            CSMWorld::IdTable::index(index, 0), CSMWorld::IdTable::index(index, mIdCollection->getColumns() - 1));
    }
}

const CSMWorld::RecordBase& CSMWorld::IdTable::getRecord(const std::string& id) const
{
    return mIdCollection->getRecord(ESM::RefId::stringRefId(id));
}

int CSMWorld::IdTable::searchColumnIndex(Columns::ColumnId id) const
{
    return mIdCollection->searchColumnIndex(id);
}

int CSMWorld::IdTable::findColumnIndex(Columns::ColumnId id) const
{
    return mIdCollection->findColumnIndex(id);
}

void CSMWorld::IdTable::reorderRows(int baseIndex, const std::vector<int>& newOrder)
{
    if (newOrder.empty())
        return;
    if (!mIdCollection->reorderRows(baseIndex, newOrder))
        return;
    emit dataChanged(
        index(baseIndex, 0), index(baseIndex + static_cast<int>(newOrder.size()) - 1, mIdCollection->getColumns() - 1));
}

std::pair<CSMWorld::UniversalId, std::string> CSMWorld::IdTable::view(int row) const
{
    std::string id;
    std::string hint;

    if (getFeatures() & Feature_ViewCell)
    {
        int cellColumn = mIdCollection->searchColumnIndex(Columns::ColumnId_Cell);
        int idColumn = mIdCollection->searchColumnIndex(Columns::ColumnId_Id);

        if (cellColumn != -1 && idColumn != -1)
        {
            id = mIdCollection->getData(row, cellColumn).toString().toUtf8().constData();
            hint = "r:" + std::string(mIdCollection->getData(row, idColumn).toString().toUtf8().constData());
        }
    }
    else if (getFeatures() & Feature_ViewId)
    {
        int column = mIdCollection->searchColumnIndex(Columns::ColumnId_Id);

        if (column != -1)
        {
            id = mIdCollection->getData(row, column).toString().toUtf8().constData();
            hint = "c:" + id;
        }
    }

    if (id.empty())
        return std::make_pair(UniversalId::Type_None, "");

    if (id[0] == '#')
        id = ESM::Cell::sDefaultWorldspaceId.getValue();

    return std::make_pair(UniversalId(UniversalId::Type_Scene, id), hint);
}

/// For top level data/columns
bool CSMWorld::IdTable::isDeleted(const std::string& id) const
{
    return getRecord(id).isDeleted();
}

int CSMWorld::IdTable::getColumnId(int column) const
{
    return mIdCollection->getColumn(column).getId();
}

CSMWorld::CollectionBase* CSMWorld::IdTable::idCollection() const
{
    return mIdCollection;
}

CSMWorld::LandTextureIdTable::LandTextureIdTable(CollectionBase* idCollection, unsigned int features)
    : IdTable(idCollection, features)
{
}

const CSMWorld::Record<ESM::LandTexture>* CSMWorld::LandTextureIdTable::searchRecord(
    std::uint16_t index, int plugin) const
{
    return static_cast<CSMWorld::IdCollection<ESM::LandTexture>*>(idCollection())->searchRecord(index, plugin);
}
