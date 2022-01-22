#include "globalcreator.hpp"

#include <components/esm3/variant.hpp>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"

namespace CSVWorld
{
    void GlobalCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const
    {
        CSMWorld::IdTable* table = static_cast<CSMWorld::IdTable*>(getData().getTableModel(getCollectionId()));

        int index = table->findColumnIndex(CSMWorld::Columns::ColumnId_ValueType);
        int type = (int)ESM::VT_Float;

        command.addValue(index, type);
    }

    GlobalCreator::GlobalCreator(CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id)
        : GenericCreator (data, undoStack, id, true)
    {
    }
}
