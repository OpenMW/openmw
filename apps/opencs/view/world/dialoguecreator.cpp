
#include "dialoguecreator.hpp"

#include <components/esm/loaddial.hpp>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"

void CSVWorld::DialogueCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const
{
    int index =
        dynamic_cast<CSMWorld::IdTable&> (*getData().getTableModel (getCollectionId())).
        findColumnIndex (CSMWorld::Columns::ColumnId_DialogueType);

    command.addValue (index, mType);
}

CSVWorld::DialogueCreator::DialogueCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id, int type)
: GenericCreator (data, undoStack, id, true), mType (type)
{}

CSVWorld::Creator *CSVWorld::TopicCreatorFactory::makeCreator (CSMWorld::Data& data,
    QUndoStack& undoStack, const CSMWorld::UniversalId& id) const
{
    return new DialogueCreator (data, undoStack, id, ESM::Dialogue::Topic);
}

CSVWorld::Creator *CSVWorld::JournalCreatorFactory::makeCreator (CSMWorld::Data& data,
    QUndoStack& undoStack, const CSMWorld::UniversalId& id) const
{
    return new DialogueCreator (data, undoStack, id, ESM::Dialogue::Journal);
}
