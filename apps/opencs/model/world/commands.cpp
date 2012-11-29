
#include "commands.hpp"

#include <QAbstractTableModel>

CSMWorld::ModifyCommand::ModifyCommand (QAbstractItemModel& model, const QModelIndex& index,
    const QVariant& new_, QUndoCommand *parent)
: QUndoCommand (parent), mModel (model), mIndex (index), mNew (new_)
{
    mOld = mModel.data (mIndex, Qt::EditRole);

    setText ("Modify " + mModel.headerData (mIndex.column(), Qt::Horizontal, Qt::DisplayRole).toString());
}

void CSMWorld::ModifyCommand::redo()
{
    mModel.setData (mIndex, mNew);
}

void CSMWorld::ModifyCommand::undo()
{
    mModel.setData (mIndex, mOld);
}