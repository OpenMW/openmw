#include "idtree.hpp"

#include "nestedtablewrapper.hpp" // FIXME: is this necessary?

#include "nestedcollection.hpp"
#include "nestablecolumn.hpp"

CSMWorld::IdTree::IdTree (NestedCollection *idCollection, unsigned int features)
: IdTable (idCollection, features), mIdCollection (idCollection)
{}

CSMWorld::IdTree::~IdTree()
{
    // FIXME: workaround only, a proper fix should stop QHideEvent calls after destruction
    mIdCollection = 0;
}

int CSMWorld::IdTree::rowCount (const QModelIndex & parent) const
{
    if (hasChildren(parent))
        return mIdCollection->getNestedRowsCount(parent.row(), parent.column());

    return mIdCollection->getSize();
}

int CSMWorld::IdTree::columnCount (const QModelIndex & parent) const
{
    if (hasChildren(parent))
        return mIdCollection->getNestedColumnsCount(parent.row(), parent.column());

    return mIdCollection->getColumns();
}

QVariant CSMWorld::IdTree::data  (const QModelIndex & index, int role) const
{
     if (!index.isValid())
          return QVariant();

    if ((role!=Qt::DisplayRole && role!=Qt::EditRole) || index.row() < 0 || index.column() < 0)
        return QVariant();

    if (role==Qt::EditRole && !mIdCollection->getColumn (index.column()).isEditable())
        return QVariant();

    if (index.internalId() != 0)
    {
        std::pair<int, int> parentAdress(unfoldIndexAdress(index.internalId()));

        return mIdCollection->getNestedData(parentAdress.first,
                                            parentAdress.second, index.row(), index.column());
    }
    else
        return mIdCollection->getData (index.row(), index.column());
}

QVariant CSMWorld::IdTree::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation==Qt::Vertical)
        return QVariant();

    if (orientation != Qt::Horizontal)
        throw std::logic_error("Unknown header orientation specified");

    if (role == Qt::DisplayRole)
        return tr (mIdCollection->getColumn (section).getTitle().c_str());

    if (role == ColumnBase::Role_Flags)
        return mIdCollection->getColumn (section).mFlags;

    if (role == ColumnBase::Role_Display)
        return mIdCollection->getColumn (section).mDisplayType;

    return QVariant();
}

QVariant CSMWorld::IdTree::nestedHeaderData(int section, int subSection, Qt::Orientation orientation, int role) const
{
    // FIXME: workaround only, a proper fix should stop QHideEvent calls after destruction
    if (section < 0 || !mIdCollection || section >= mIdCollection->getColumns())
        return QVariant();

    // FIXME: dynamic cast
    const NestableColumn& parentColumn = dynamic_cast<const NestableColumn&>(mIdCollection->getColumn(section));

    if (orientation==Qt::Vertical)
        return QVariant();

    if (role==Qt::DisplayRole)
        return tr(parentColumn.nestedColumn(subSection).getTitle().c_str());

    if (role==ColumnBase::Role_Flags)
        return mIdCollection->getColumn (section).mFlags;

    if (role==ColumnBase::Role_Display)
        return parentColumn.nestedColumn(subSection).mDisplayType;

    return QVariant();
}

bool CSMWorld::IdTree::setData (const QModelIndex &index, const QVariant &value, int role)
{
    if (index.internalId() != 0)
    {
        if (mIdCollection->getColumn(parent(index).column()).isEditable() && role==Qt::EditRole)
        {
            const std::pair<int, int>& parentAdress(unfoldIndexAdress(index.internalId()));

            mIdCollection->setNestedData(parentAdress.first, parentAdress.second, value, index.row(), index.column());

            emit dataChanged (CSMWorld::IdTree::index (parentAdress.first, 0),
                              CSMWorld::IdTree::index (parentAdress.second, mIdCollection->getColumns()-1));

            return true;
        }
        else
            return false;
    }

    if (mIdCollection->getColumn (index.column()).isEditable() && role==Qt::EditRole)
    {
        mIdCollection->setData (index.row(), index.column(), value);

        emit dataChanged (CSMWorld::IdTree::index (index.row(), 0),
                          CSMWorld::IdTree::index (index.row(), mIdCollection->getColumns()-1));

        return true;
    }

    return false;
}

Qt::ItemFlags CSMWorld::IdTree::flags (const QModelIndex & index) const
{
    if (!index.isValid())
        return 0;

    Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

    if (mIdCollection->getColumn (index.column()).isUserEditable())
        flags |= Qt::ItemIsEditable;

    return flags;
}

bool CSMWorld::IdTree::removeRows (int row, int count, const QModelIndex& parent)
{
    beginRemoveRows (parent, row, row+count-1);

    if (parent.isValid())
    {
        for (int i = 0; i < count; ++i)
        {
            mIdCollection->removeNestedRows(parent.row(), parent.column(), row+i);
        }
    }
    else
    {

        beginRemoveRows (parent, row, row+count-1);

        mIdCollection->removeRows (row, count);
    }

    endRemoveRows();

    emit dataChanged (CSMWorld::IdTree::index (parent.row(), 0),
                      CSMWorld::IdTree::index (parent.row(), mIdCollection->getColumns()-1));

    return true;
}

void CSMWorld::IdTree::addNestedRow(const QModelIndex& parent, int position)
{
    if (!hasChildren(parent))
        throw std::logic_error("Tried to set nested table, but index has no children");

    int row = parent.row();

    beginInsertRows(parent, position, position);
    mIdCollection->addNestedRow(row, parent.column(), position);
    endInsertRows();

    emit dataChanged (CSMWorld::IdTree::index (row, 0),
                      CSMWorld::IdTree::index (row, mIdCollection->getColumns()-1));
}

QModelIndex CSMWorld::IdTree::index (int row, int column, const QModelIndex& parent) const
{
    unsigned int encodedId = 0;
    if (parent.isValid())
    {
        encodedId = this->foldIndexAdress(parent);
    }

    if (row<0 || row>=mIdCollection->getSize())
        return QModelIndex();

    if (column<0 || column>=mIdCollection->getColumns())
        return QModelIndex();

    return createIndex(row, column, encodedId); // store internal id
}

QModelIndex CSMWorld::IdTree::parent (const QModelIndex& index) const
{
    if (index.internalId() == 0) // 0 is used for indexs with invalid parent (top level data)
        return QModelIndex();

    unsigned int id = index.internalId();
    const std::pair<int, int>& adress(unfoldIndexAdress(id));

    if (adress.first >= this->rowCount() || adress.second >= this->columnCount())
        throw "Parent index is not present in the model";

    return createIndex(adress.first, adress.second);
}

void CSMWorld::IdTree::setRecord (const std::string& id, const RecordBase& record)
{
    int index = mIdCollection->searchId (id);

    if (index==-1)
    {
        int index = mIdCollection->getAppendIndex (id);

        beginInsertRows (QModelIndex(), index, index);

        mIdCollection->appendRecord (record);

        endInsertRows();
    }
    else
    {
        mIdCollection->replace (index, record);
        emit dataChanged (CSMWorld::IdTree::index (index, 0),
            CSMWorld::IdTree::index (index, mIdCollection->getColumns()-1));
    }
}

unsigned int CSMWorld::IdTree::foldIndexAdress (const QModelIndex& index) const
{
    unsigned int out = index.row() * this->columnCount();
    out += index.column();
    return ++out;
}

std::pair< int, int > CSMWorld::IdTree::unfoldIndexAdress (unsigned int id) const
{
    if (id == 0)
        throw "Attempt to unfold index id of the top level data cell";

    --id;
    int row = id / this->columnCount();
    int column = id - row * this->columnCount();
    return std::make_pair (row, column);
}

bool CSMWorld::IdTree::hasChildren(const QModelIndex& index) const
{
    // FIXME: dynamic cast
    return (index.isValid() &&
            index.internalId() == 0 &&
            dynamic_cast<const CSMWorld::NestableColumn &>(mIdCollection->getColumn(index.column())).hasChildren() &&
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

    mIdCollection->setNestedTable(index.row(), index.column(), nestedTable);

    emit dataChanged (CSMWorld::IdTree::index (index.row(), 0),
                      CSMWorld::IdTree::index (index.row(), mIdCollection->getColumns()-1));

    if (removeRowsMode)
    {
        emit resetEnd(this->index(index.row(), 0).data().toString());
    }
}

CSMWorld::NestedTableWrapperBase* CSMWorld::IdTree::nestedTable(const QModelIndex& index) const
{
    if (!hasChildren(index))
        throw std::logic_error("Tried to retrive nested table, but index has no children");

    return mIdCollection->nestedTable(index.row(), index.column());
}
