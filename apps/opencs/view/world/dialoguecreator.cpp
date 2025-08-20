#include "dialoguecreator.hpp"

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/view/world/genericcreator.hpp>

#include <components/esm3/loaddial.hpp>

#include "../../model/world/commands.hpp"
#include "../../model/world/idtable.hpp"

class QUndoStack;

void CSVWorld::DialogueCreator::configureCreateCommand(CSMWorld::CreateCommand& command) const
{
    int index = dynamic_cast<CSMWorld::IdTable&>(*getData().getTableModel(getCollectionId()))
                    .findColumnIndex(CSMWorld::Columns::ColumnId_DialogueType);

    command.addValue(index, mType);
}

CSVWorld::DialogueCreator::DialogueCreator(
    CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id, int type)
    : GenericCreator(worldData, undoStack, id, true)
    , mType(type)
{
}

CSVWorld::Creator* CSVWorld::TopicCreatorFactory::makeCreator(
    CSMDoc::Document& document, const CSMWorld::UniversalId& id) const
{
    return new DialogueCreator(document.getData(), document.getUndoStack(), id, ESM::Dialogue::Topic);
}

CSVWorld::Creator* CSVWorld::JournalCreatorFactory::makeCreator(
    CSMDoc::Document& document, const CSMWorld::UniversalId& id) const
{
    return new DialogueCreator(document.getData(), document.getUndoStack(), id, ESM::Dialogue::Journal);
}
