#ifndef CSV_WORLD_DIALOGUECREATOR_H
#define CSV_WORLD_DIALOGUECREATOR_H

#include "genericcreator.hpp"

namespace CSVWorld
{
    class DialogueCreator : public GenericCreator
    {
            int mType;

        protected:

            void configureCreateCommand (CSMWorld::CreateCommand& command) const override;

        public:

            DialogueCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id, int type);
    };

    class TopicCreatorFactory : public CreatorFactoryBase
    {
        public:

            Creator *makeCreator (CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
            ///< The ownership of the returned Creator is transferred to the caller.
    };

    class JournalCreatorFactory : public CreatorFactoryBase
    {
        public:

            Creator *makeCreator (CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
            ///< The ownership of the returned Creator is transferred to the caller.
    };
}

#endif
