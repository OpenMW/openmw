#ifndef CSV_WORLD_INFOCREATOR_H
#define CSV_WORLD_INFOCREATOR_H

#include "genericcreator.hpp"

class QLineEdit;

namespace CSMWorld
{
    class InfoCollection;
}

namespace CSVWorld
{
    class InfoCreator : public GenericCreator
    {
            Q_OBJECT

            QLineEdit *mTopic;

            virtual std::string getId() const;

            virtual void configureCreateCommand (CSMWorld::CreateCommand& command) const;

        public:

            InfoCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            virtual void cloneMode (const std::string& originId,
                const CSMWorld::UniversalId::Type type);

            virtual void reset();

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.
            
            /// Focus main input widget
            virtual void focus();
            
        private slots:

            void topicChanged();
    };
}

#endif
