#ifndef CSV_WORLD_REFERENCECREATOR_H
#define CSV_WORLD_REFERENCECREATOR_H

#include "genericcreator.hpp"

class QLineEdit;

namespace CSVWorld
{
    class ReferenceCreator : public GenericCreator
    {
            Q_OBJECT

            QLineEdit *mCell;
            std::string mId;

        private:

            virtual std::string getId() const;

            virtual void configureCreateCommand (CSMWorld::CreateCommand& command) const;

        public:

            ReferenceCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            virtual void reset();

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.

        private slots:

            void cellChanged();
    };
}

#endif
