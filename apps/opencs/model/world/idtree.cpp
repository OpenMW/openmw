#include "idtree.hpp"

#include "nestedtablewrapper.hpp"

#include "collectionbase.hpp"
#include "nestedcollection.hpp"
#include "columnbase.hpp"

// NOTE: parent class still needs idCollection
CSMWorld::IdTree::IdTree (NestedCollection *nestedCollection, CollectionBase *idCollection, unsigned int features)
: IdTable (idCollection, features), mNestedCollection (nestedCollection)
{}

CSMWorld::IdTree::~IdTree()
{}

int CSMWorld::IdTree::rowCount (const QModelIndex & parent) const
{
    if (hasChildren(parent))
        return mNestedCollection->getNestedRowsCount(parent.row(), parent.column());

    return IdTable::rowCount(parent);
}

int CSMWorld::IdTree::columnCount (const QModelIndex & parent) const
{
    if (hasChildren(parent))
        return mNestedCollection->getNestedColumnsCount(parent.row(), parent.column());

    return IdTable::columnCount(parent);
}

QVariant CSMWorld::IdTree::data  (const QModelIndex & index, int role) const
{
     if (!index.isValid())
          return QVariant();

    if ((role!=Qt::DisplayRole && role!=Qt::EditRole) || index.row() < 0 || index.column() < 0)
        return QVariant();

    if (index.internalId() != 0)
    {
        std::pair<int, int> parentAddress(unfoldIndexAddress(index.internalId()));

        if (role == Qt::EditRole &&
            !mNestedCollection->getNestableColumn(parentAddress.second)->nestedColumn(index.column()).isEditable())
        {
            return QVariant();
        }

        return mNestedCollection->getNestedData(parentAddress.first,
                                            parentAddress.second, index.row(), index.column());
    }
    else
    {
        if (role==Qt::EditRole && !idCollection()->getColumn (index.column()).isEditable())
            return QVariant();

        return idCollection()->getData (index.row(), index.column());
    }
}

QVariant CSMWorld::IdTree::nestedHeaderData(int section, int subSection, Qt::Orientation orientation, int role) const
{
    if (section < 0 || section >= idCollection()->getColumns())
        return QVariant();

    const NestableColumn *parentColumn = mNestedCollection->getNestableColumn(section);

    if (orientation==Qt::Vertical)
        return QVariant();

    if (role==Qt::DisplayRole)
        return tr(parentColumn->nestedColumn(subSection).getTitle().c_str());

    if (role==ColumnBase::Role_Flags)
        return idCollection()->getColumn (section).mFlags;

    if (role==ColumnBase::Role_Display)
        return parentColumn->nestedColumn(subSection).mDisplayType;

    return QVariant();
}

bool CSMWorld::IdTree::setData (const QModelIndex &index, const QVariant &value, int role)
{
    if (index.internalId() != 0)
    {
        if (idCollection()->getColumn(parent(index).column()).isEditable() && role==Qt::EditRole)
        {
            const std::pair<int, int>& parentAddress(unfoldIndexAddress(index.internalId()));

            mNestedCollection->setNestedData(parentAddress.first, parentAddress.second, value, index.row(), index.column());

            emit dataChanged (CSMWorld::IdTree::index (parentAddress.first, 0),
                              CSMWorld::IdTree::index (parentAddress.first, idCollection()->getColumns()-1));
            return true;
        }
        else
            return false;
    }
    return IdTable::setData(index, value, role);
}

Qt::ItemFlags CSMWorld::IdTree::flags (const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    if (index.internalId() != 0)
    {
        std::pair<int, int> parentAddress(unfoldIndexAddress(index.internalId()));

        Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

        if (mNestedCollection->getNestableColumn(parentAddress.second)->nestedColumn(index.column()).isEditable())
            flags |= Qt::ItemIsEditable;

        return flags;
    }
    else
        return IdTable::flags(index);
}

bool CSMWorld::IdTree::removeRows (int row, int count, const QModelIndex& parent)
{
    if (parent.isValid())
    {
        beginRemoveRows (parent, row, row+count-1);

        for (int i = 0; i < count; ++i)
        {
            mNestedCollection->removeNestedRows(parent.row(), parent.column(), row+i);
        }

        endRemoveRows();

        emit dataChanged (CSMWorld::IdTree::index (parent.row(), 0),
                          CSMWorld::IdTree::index (parent.row(), idCollection()->getColumns()-1));

        return true;
    }
    else
        return IdTable::removeRows(row, count, parent);
}

void CSMWorld::IdTree::addNestedRow(const QModelIndex& parent, int position)
{
    if (!hasChildren(parent))
        throw std::logic_error("Tried to set nested table, but index has no children");

    int row = parent.row();

    beginInsertRows(parent, position, position);
    mNestedCollection->addNestedRow(row, parent.column(), position);
    endInsertRows();

    emit dataChanged (CSMWorld::IdTree::index (row, 0),
                      CSMWorld::IdTree::index (row, idCollection()->getColumns()-1));
}

QModelIndex CSMWorld::IdTree::index (int row, int column, const QModelIndex& parent) const
{
    unsigned int encodedId = 0;
    if (parent.isValid())
    {
        encodedId = this->foldIndexAddress(parent);
    }

    if (row<0 || row>=idCollection()->getSize())
        return QModelIndex();

    if (column<0 || column>=idCollection()->getColumns())
        return QModelIndex();

    return createIndex(row, column, encodedId); // store internal id
}

QModelIndex CSMWorld::IdTree::getNestedModelIndex (const std::string& id, int column) const
{
    return CSMWorld::IdTable::index(idCollection()->getIndex (id), column);
}

QModelIndex CSMWorld::IdTree::parent (const QModelIndex& index) const
{
    if (index.internalId() == 0) // 0 is used for indexs with invalid parent (top level data)
        return QModelIndex();

    unsigned int id = index.internalId();
    const std::pair<int, int>& adress(unfoldIndexAddress(id));

    if (adress.first >= this->rowCount() || adress.second >= this->columnCount())
        throw "Parent index is not present in the model";

    return createIndex(adress.first, adress.second);
}

unsigned int CSMWorld::IdTree::foldIndexAddress (const QModelIndex& index) const
{
    unsigned int out = index.row() * this->columnCount();
    out += index.column();
    return ++out;
}

std::pair< int, int > CSMWorld::IdTree::unfoldIndexAddress (unsigned int id) const
{
    if (id == 0)
        throw "Attempt to unfold index id of the top level data cell";

    --id;
    int row = id / this->columnCount();
    int column = id - row * this->columnCount();
    return std::make_pair (row, column);
}

// FIXME: Not sure why this check is also needed?
//
// index.data().isValid() requires RefIdAdapter::getData() to return a valid QVariant for
// nested columns (refidadapterimp.hpp)
//
// Also see comments in refidadapter.hpp and refidadapterimp.hpp.
bool CSMWorld::IdTree::hasChildren(const QModelIndex& index) const
{
    return (index.isValid() &&
            index.internalId() == 0 &&
            mNestedCollection->getNestableColumn(index.column())->hasChildren() &&
            index.data().isValid());
}

void CSMWorld::IdTree::setNestedTable(const QModelIndex& index, const CSMWorld::NestedTableWrapperBase& nestedTable)
{
    if (!hasChildren(index))
        throw std::logic_error("Tried to set nested table, but index has no children");

    bool removeRowsMode = false;
    if (nestedTable.size() != this->nestedTable(index)->size())
    {
        emit resetStart(this->index(index.row(), 0).data().toString());
        removeRowsMode = true;
    }

    mNestedCollection->setNestedTable(index.row(), index.column(), nestedTable);

    emit dataChanged (CSMWorld::IdTree::index (index.row(), 0),
                      CSMWorld::IdTree::index (index.row(), idCollection()->getColumns()-1));

    if (removeRowsMode)
    {
        emit resetEnd(this->index(index.row(), 0).data().toString());
    }
}

CSMWorld::NestedTableWrapperBase* CSMWorld::IdTree::nestedTable(const QModelIndex& index) const
{
    if (!hasChildren(index))
        throw std::logic_error("Tried to retrive nested table, but index has no children");

    return mNestedCollection->nestedTable(index.row(), index.column());
}
