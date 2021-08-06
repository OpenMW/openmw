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

            void configureCreateCommand (CSMWorld::CreateCommand& command) const override;

        public:

            ReferenceableCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            void reset() override;

            void cloneMode (const std::string& originId,
                const CSMWorld::UniversalId::Type type) override;

            void toggleWidgets(bool active = true) override;

        private slots:

            void setType (int index);
    };
}

#endif
