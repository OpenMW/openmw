#include "commands.hpp"

#include <cmath>
#include <sstream>

#include <components/misc/stringops.hpp>

#include <QAbstractItemModel>
#include <QAbstractProxyModel>

#include "idtable.hpp"
#include "idtree.hpp"
#include "nestedtablewrapper.hpp"

CSMWorld::ModifyCommand::ModifyCommand (QAbstractItemModel& model, const QModelIndex& index,
                                        const QVariant& new_, QUndoCommand* parent)
: QUndoCommand (parent), mModel (&model), mIndex (index), mNew (new_), mHasRecordState(false)
{
    if (QAbstractProxyModel *proxy = dynamic_cast<QAbstractProxyModel *> (&model))
    {
        // Replace proxy with actual model
        mIndex = proxy->mapToSource (index);
        mModel = proxy->sourceModel();
    }

    if (mIndex.parent().isValid())
    {
        setText ("Modify " + dynamic_cast<CSMWorld::IdTree*>(mModel)->nestedHeaderData (
                    mIndex.parent().column(), mIndex.column(), Qt::Horizontal, Qt::DisplayRole).toString());
    }
    else
    {
        setText ("Modify " + mModel->headerData (mIndex.column(), Qt::Horizontal, Qt::DisplayRole).toString());
    }

    // Remember record state before the modification
    if (CSMWorld::IdTable *table = dynamic_cast<IdTable *>(mModel))
    {
        mHasRecordState = true;
        int stateColumnIndex = table->findColumnIndex(Columns::ColumnId_Modification);

        int rowIndex = mIndex.row();
        if (mIndex.parent().isValid())
        {
            rowIndex = mIndex.parent().row();
        }

        mRecordStateIndex = table->index(rowIndex, stateColumnIndex);
        mOldRecordState = static_cast<CSMWorld::RecordBase::State>(table->data(mRecordStateIndex).toInt());
    }
}

void CSMWorld::ModifyCommand::redo()
{
    mOld = mModel->data (mIndex, Qt::EditRole);
    mModel->setData (mIndex, mNew);
}

void CSMWorld::ModifyCommand::undo()
{
    mModel->setData (mIndex, mOld);
    if (mHasRecordState)
    {
        mModel->setData(mRecordStateIndex, mOldRecordState);
    }
}


void CSMWorld::CreateCommand::applyModifications()
{
    for (std::map<int, QVariant>::const_iterator iter (mValues.begin()); iter!=mValues.end(); ++iter)
        mModel.setData (mModel.getModelIndex (mId, iter->first), iter->second);

    if (!mNestedValues.empty())
    {
        CSMWorld::IdTree *tree = dynamic_cast<CSMWorld::IdTree *>(&mModel);
        if (tree == NULL)
        {
            throw std::logic_error("CSMWorld::CreateCommand: Attempt to add nested values to the non-nested model");
        }

        std::map<int, std::pair<int, QVariant> >::const_iterator current = mNestedValues.begin();
        std::map<int, std::pair<int, QVariant> >::const_iterator end = mNestedValues.end();
        for (; current != end; ++current)
        {
            QModelIndex index = tree->index(0,
                                            current->second.first,
                                            tree->getNestedModelIndex(mId, current->first));
            tree->setData(index, current->second.second);
        }
    }
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

void CSMWorld::CreateCommand::addNestedValue(int parentColumn, int nestedColumn, const QVariant &value)
{
    mNestedValues[parentColumn] = std::make_pair(nestedColumn, value);
}

void CSMWorld::CreateCommand::setType (UniversalId::Type type)
{
    mType = type;
}

void CSMWorld::CreateCommand::redo()
{
    mModel.addRecord (mId, mType);
    applyModifications();
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

CSMWorld::DeleteCommand::DeleteCommand (IdTable& model,
        const std::string& id, CSMWorld::UniversalId::Type type, QUndoCommand* parent)
: QUndoCommand (parent), mModel (model), mId (id), mOld (0), mType(type)
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
    mModel.setRecord (mId, *mOld, mType);
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
                                      const std::string& idDestination,
                                      const CSMWorld::UniversalId::Type type,
                                      QUndoCommand* parent)
: CreateCommand (model, idDestination, parent), mIdOrigin (idOrigin)
{
    setType (type);
    setText ( ("Clone record " + idOrigin + " to the " + idDestination).c_str());
}

void CSMWorld::CloneCommand::redo()
{
    mModel.cloneRecord (mIdOrigin, mId, mType);
    applyModifications();
}

void CSMWorld::CloneCommand::undo()
{
    mModel.removeRow (mModel.getModelIndex (mId, 0).row());
}


CSMWorld::UpdateCellCommand::UpdateCellCommand (IdTable& model, int row, QUndoCommand *parent)
: QUndoCommand (parent), mModel (model), mRow (row)
{
    setText ("Update cell ID");
}

void CSMWorld::UpdateCellCommand::redo()
{
    if (!mNew.isValid())
    {
        int cellColumn = mModel.searchColumnIndex (Columns::ColumnId_Cell);
        mIndex = mModel.index (mRow, cellColumn);

        const int cellSize = 8192;

        QModelIndex xIndex = mModel.index (
            mRow, mModel.findColumnIndex (Columns::ColumnId_PositionXPos));

        QModelIndex yIndex = mModel.index (
            mRow, mModel.findColumnIndex (Columns::ColumnId_PositionYPos));

        int x = std::floor (mModel.data (xIndex).toFloat() / cellSize);
        int y = std::floor (mModel.data (yIndex).toFloat() / cellSize);

        std::ostringstream stream;

        stream << "#" << x << " " << y;

        mNew = QString::fromUtf8 (stream.str().c_str());
    }

    mModel.setData (mIndex, mNew);
}

void CSMWorld::UpdateCellCommand::undo()
{
    mModel.setData (mIndex, mOld);
}


CSMWorld::DeleteNestedCommand::DeleteNestedCommand (IdTree& model,
                                                    const std::string& id,
                                                    int nestedRow,
                                                    int parentColumn,
                                                    QUndoCommand* parent) :
    QUndoCommand(parent),
    NestedTableStoring(model, id, parentColumn),
    mModel(model),
    mId(id),
    mParentColumn(parentColumn),
    mNestedRow(nestedRow)
{
    std::string title =
        model.headerData(parentColumn, Qt::Horizontal, Qt::DisplayRole).toString().toUtf8().constData();
    setText (("Delete row in " + title + " sub-table of " + mId).c_str());

    QModelIndex parentIndex = mModel.getModelIndex(mId, mParentColumn);
    mModifyParentCommand = new ModifyCommand(mModel, parentIndex, parentIndex.data(Qt::EditRole), this);
}

void CSMWorld::DeleteNestedCommand::redo()
{
    QModelIndex parentIndex = mModel.getModelIndex(mId, mParentColumn);
    mModel.removeRows (mNestedRow, 1, parentIndex);
    mModifyParentCommand->redo();
}


void CSMWorld::DeleteNestedCommand::undo()
{
    QModelIndex parentIndex = mModel.getModelIndex(mId, mParentColumn);
    mModel.setNestedTable(parentIndex, getOld());
    mModifyParentCommand->undo();
}

CSMWorld::AddNestedCommand::AddNestedCommand(IdTree& model, const std::string& id, int nestedRow, int parentColumn, QUndoCommand* parent)
    : QUndoCommand(parent),
      NestedTableStoring(model, id, parentColumn),
      mModel(model),
      mId(id),
      mNewRow(nestedRow),
      mParentColumn(parentColumn)
{
    std::string title =
        model.headerData(parentColumn, Qt::Horizontal, Qt::DisplayRole).toString().toUtf8().constData();
    setText (("Add row in " + title + " sub-table of " + mId).c_str());

    QModelIndex parentIndex = mModel.getModelIndex(mId, mParentColumn);
    mModifyParentCommand = new ModifyCommand(mModel, parentIndex, parentIndex.data(Qt::EditRole), this);
}

void CSMWorld::AddNestedCommand::redo()
{
    QModelIndex parentIndex = mModel.getModelIndex(mId, mParentColumn);
    mModel.addNestedRow (parentIndex, mNewRow);
    mModifyParentCommand->redo();
}

void CSMWorld::AddNestedCommand::undo()
{
    QModelIndex parentIndex = mModel.getModelIndex(mId, mParentColumn);
    mModel.setNestedTable(parentIndex, getOld());
    mModifyParentCommand->undo();
}

CSMWorld::NestedTableStoring::NestedTableStoring(const IdTree& model, const std::string& id, int parentColumn)
    : mOld(model.nestedTable(model.getModelIndex(id, parentColumn))) {}

CSMWorld::NestedTableStoring::~NestedTableStoring()
{
    delete mOld;
}

const CSMWorld::NestedTableWrapperBase& CSMWorld::NestedTableStoring::getOld() const
{
    return *mOld;
}
