
#include "commands.hpp"

#include <QAbstractTableModel>

#include "idtableproxymodel.hpp"

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

CSMWorld::CreateCommand::CreateCommand (IdTableProxyModel& model, const std::string& id, QUndoCommand *parent)
: QUndoCommand (parent), mModel (model), mId (id)
{
    setText (("Create record " + id).c_str());
}

void CSMWorld::CreateCommand::redo()
{
    mModel.addRecord (mId);
}

void CSMWorld::CreateCommand::undo()
{
    mModel.removeRow (mModel.getModelIndex (mId, 0).row());
}