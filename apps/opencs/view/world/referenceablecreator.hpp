#ifndef CSV_WORLD_REFERENCEABLECREATOR_H
#define CSV_WORLD_REFERENCEABLECREATOR_H

class QComboBox;

#include "genericcreator.hpp"

namespace CSVWorld
{
    class ReferenceableCreator : public GenericCreator
    {
            Q_OBJECT

            QComboBox *mType;

        private:

            virtual void configureCreateCommand (CSMWorld::CreateCommand& command) const;

        public:

            ReferenceableCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            virtual void reset();

            virtual void cloneMode (const std::string& originId,
                const CSMWorld::UniversalId::Type type);

            virtual void toggleWidgets(bool active = true);

    };
}

#endif
