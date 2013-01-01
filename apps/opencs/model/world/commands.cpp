
#include "commands.hpp"

#include <QAbstractTableModel>

#include "idtableproxymodel.hpp"
#include "idtable.hpp"

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

CSMWorld::RevertCommand::RevertCommand (IdTable& model, const std::string& id, QUndoCommand *parent)
: QUndoCommand (parent), mModel (model), mId (id), mOld (0)
{
    setText (("Revert record " + id).c_str());

    mOld = model.getRecord (id).clone();
}

CSMWorld::RevertCommand::~RevertCommand()
{
    delete mOld;
}

void CSMWorld::RevertCommand::redo()
{
    QModelIndex index = mModel.getModelIndex (mId, 1);
    RecordBase::State state = static_cast<RecordBase::State> (mModel.data (index).toInt());

    if (state==RecordBase::State_ModifiedOnly)
    {
        mModel.removeRows (index.row(), 1);
    }
    else
    {
        mModel.setData (index, static_cast<int> (RecordBase::State_BaseOnly));
    }
}

void CSMWorld::RevertCommand::undo()
{
    mModel.setRecord (*mOld);
}

CSMWorld::DeleteCommand::DeleteCommand (IdTable& model, const std::string& id, QUndoCommand *parent)
: QUndoCommand (parent), mModel (model), mId (id), mOld (0)
{
    setText (("Delete record " + id).c_str());

    mOld = model.getRecord (id).clone();
}

CSMWorld::DeleteCommand::~DeleteCommand()
{
    delete mOld;
}

void CSMWorld::DeleteCommand::redo()
{
    QModelIndex index = mModel.getModelIndex (mId, 1);
    RecordBase::State state = static_cast<RecordBase::State> (mModel.data (index).toInt());

    if (state==RecordBase::State_ModifiedOnly)
    {
        mModel.removeRows (index.row(), 1);
    }
    else
    {
        mModel.setData (index, static_cast<int> (RecordBase::State_Deleted));
    }
}

void CSMWorld::DeleteCommand::undo()
{
    mModel.setRecord (*mOld);
}