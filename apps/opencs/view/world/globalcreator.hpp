#ifndef CSV_WORLD_GLOBALCREATOR_H
#define CSV_WORLD_GLOBALCREATOR_H

#include "genericcreator.hpp"

namespace CSVWorld
{
    class GlobalCreator : public GenericCreator
    {
            Q_OBJECT

        public:

            GlobalCreator(CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

        protected:

            void configureCreateCommand(CSMWorld::CreateCommand& command) const override;
    };
}

#endif
