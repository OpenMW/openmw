
#include "util.hpp"

#include <QUndoStack>

#include "../../model/world/commands.hpp"

CSVWorld::NastyTableModelHack::NastyTableModelHack (QAbstractItemModel& model)
: mModel (model)
{}

int CSVWorld::NastyTableModelHack::rowCount (const QModelIndex & parent) const
{
    return mModel.rowCount (parent);
}

int CSVWorld::NastyTableModelHack::columnCount (const QModelIndex & parent) const
{
    return mModel.columnCount (parent);
}

QVariant CSVWorld::NastyTableModelHack::data  (const QModelIndex & index, int role) const
{
    return mModel.data (index, role);
}

bool CSVWorld::NastyTableModelHack::setData ( const QModelIndex &index, const QVariant &value, int role)
{
    mData = value;
    return true;
}

QVariant CSVWorld::NastyTableModelHack::getData() const
{
    return mData;
}

CSVWorld::CommandDelegate::CommandDelegate (QUndoStack& undoStack, QObject *parent)
: QStyledItemDelegate (parent), mUndoStack (undoStack), mEditLock (false)
{}

void CSVWorld::CommandDelegate::setModelData (QWidget *editor, QAbstractItemModel *model,
        const QModelIndex& index) const
{
    if (!mEditLock)
    {
        NastyTableModelHack hack (*model);
        QStyledItemDelegate::setModelData (editor, &hack, index);
        mUndoStack.push (new CSMWorld::ModifyCommand (*model, index, hack.getData()));
    }
    ///< \todo provide some kind of feedback to the user, indicating that editing is currently not possible.
}

void  CSVWorld::CommandDelegate::setEditLock (bool locked)
{
    mEditLock = locked;
}