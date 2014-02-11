
#include "commands.hpp"

#include <QAbstractItemModel>

#include "idtable.hpp"
#include <components/misc/stringops.hpp>

CSMWorld::ModifyCommand::ModifyCommand (QAbstractItemModel& model, const QModelIndex& index,
                                        const QVariant& new_, QUndoCommand* parent)
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

CSMWorld::CreateCommand::CreateCommand (IdTable& model, const std::string& id, QUndoCommand* parent)
: QUndoCommand (parent), mModel (model), mId (id), mType (UniversalId::Type_None)
{
    setText (("Create record " + id).c_str());
}

void CSMWorld::CreateCommand::addValue (int column, const QVariant& value)
{
    mValues[column] = value;
}

void CSMWorld::CreateCommand::setType (UniversalId::Type type)
{
    mType = type;
}

void CSMWorld::CreateCommand::redo()
{
    mModel.addRecord (mId, mType);

    for (std::map<int, QVariant>::const_iterator iter (mValues.begin()); iter!=mValues.end(); ++iter)
        mModel.setData (mModel.getModelIndex (mId, iter->first), iter->second);
}

void CSMWorld::CreateCommand::undo()
{
    mModel.removeRow (mModel.getModelIndex (mId, 0).row());
}

CSMWorld::RevertCommand::RevertCommand (IdTable& model, const std::string& id, QUndoCommand* parent)
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
    int column = mModel.findColumnIndex (Columns::ColumnId_Modification);

    QModelIndex index = mModel.getModelIndex (mId, column);
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
    mModel.setRecord (mId, *mOld);
}

CSMWorld::DeleteCommand::DeleteCommand (IdTable& model, const std::string& id, QUndoCommand* parent)
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
    int column = mModel.findColumnIndex (Columns::ColumnId_Modification);

    QModelIndex index = mModel.getModelIndex (mId, column);
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
    mModel.setRecord (mId, *mOld);
}


CSMWorld::ReorderRowsCommand::ReorderRowsCommand (IdTable& model, int baseIndex,
        const std::vector<int>& newOrder)
: mModel (model), mBaseIndex (baseIndex), mNewOrder (newOrder)
{}

void CSMWorld::ReorderRowsCommand::redo()
{
    mModel.reorderRows (mBaseIndex, mNewOrder);
}

void CSMWorld::ReorderRowsCommand::undo()
{
    int size = static_cast<int> (mNewOrder.size());
    std::vector<int> reverse (size);

    for (int i=0; i< size; ++i)
        reverse.at (mNewOrder[i]) = i;

    mModel.reorderRows (mBaseIndex, reverse);
}

CSMWorld::CloneCommand::CloneCommand (CSMWorld::IdTable& model,
                                      const std::string& idOrigin,
                                      const std::string& IdDestination,
                                      const CSMWorld::UniversalId::Type type,
                                      QUndoCommand* parent) :
    QUndoCommand (parent),
    mModel (model),
    mIdOrigin (idOrigin),
    mIdDestination (Misc::StringUtils::lowerCase (IdDestination)),
    mType (type)
{
    setText ( ("Clone record " + idOrigin + " to the " + IdDestination).c_str());
}

void CSMWorld::CloneCommand::redo()
{
    mModel.cloneRecord (mIdOrigin, mIdDestination, mType);

    for (std::map<int, QVariant>::const_iterator iter (mValues.begin()); iter != mValues.end(); ++iter)
        mModel.setData (mModel.getModelIndex (mIdDestination, iter->first), iter->second);
}

void CSMWorld::CloneCommand::undo()
{
    mModel.removeRow (mModel.getModelIndex (mIdDestination, 0).row());
}