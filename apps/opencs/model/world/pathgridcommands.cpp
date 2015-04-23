#include "pathgridcommands.hpp"

#include "../../view/render/cell.hpp"

#include "idtree.hpp"
#include "nestedtablewrapper.hpp"

// Current interface does not allow adding a non-blank row, so we're forced to modify
// the whole record.
CSMWorld::ModifyPathgridCommand::ModifyPathgridCommand(IdTree& model,
    const std::string& id, int parentColumn, NestedTableWrapperBase* newRecord, QUndoCommand* parent)
    : mModel(model), mId(id), mParentColumn(parentColumn), mRecord(newRecord)
    , QUndoCommand(parent), NestedTableStoring(model, id, parentColumn)
{
    setText (("Modify Pathgrid record " + mId).c_str()); // FIXME: better description
}

void CSMWorld::ModifyPathgridCommand::redo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.setNestedTable(parentIndex, *mRecord);
}

void CSMWorld::ModifyPathgridCommand::undo()
{
    const QModelIndex& parentIndex = mModel.getModelIndex(mId, mParentColumn);

    mModel.setNestedTable(parentIndex, getOld());

    emit undoActioned();
}

void CSMWorld::SignalHandler::rebuildPathgrid()
{
    mParent->clearPathgrid();
    mParent->buildPathgrid();

    emit flagAsModified();
}

CSMWorld::SignalHandler::SignalHandler (CSVRender::Cell *parent) : mParent(parent)
{}

void CSMWorld::SignalHandler::connectToCommand(const CSMWorld::ModifyPathgridCommand *command)
{
    connect (command, SIGNAL(undoActioned()), this, SLOT(rebuildPathgrid()));
}
