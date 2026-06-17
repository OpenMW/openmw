#ifndef CSV_WORLD_GLOBALCREATOR_H
#define CSV_WORLD_GLOBALCREATOR_H

#include "genericcreator.hpp"

class QObject;
class QUndoStack;

#include <apps/opencs/model/world/universalid.hpp>

namespace CSMWorld
{
    class CreateCommand;
    class Data;
}

namespace CSVWorld
{
    class GlobalCreator : public GenericCreator
    {
        Q_OBJECT

    public:
        explicit GlobalCreator(CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

    protected:
        void configureCreateCommand(CSMWorld::CreateCommand& command) const override;
    };
}

#endif
