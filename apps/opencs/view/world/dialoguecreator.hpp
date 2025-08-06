#ifndef CSV_WORLD_DIALOGUECREATOR_H
#define CSV_WORLD_DIALOGUECREATOR_H

#include "genericcreator.hpp"

#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/world/creator.hpp>

class QUndoStack;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class CreateCommand;
    class Data;
}

namespace CSVWorld
{
    class DialogueCreator : public GenericCreator
    {
        int mType;

    protected:
        void configureCreateCommand(CSMWorld::CreateCommand& command) const override;

    public:
        explicit DialogueCreator(
            CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id, int type);
    };

    class TopicCreatorFactory : public CreatorFactoryBase
    {
    public:
        Creator* makeCreator(CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
        ///< The ownership of the returned Creator is transferred to the caller.
    };

    class JournalCreatorFactory : public CreatorFactoryBase
    {
    public:
        Creator* makeCreator(CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
        ///< The ownership of the returned Creator is transferred to the caller.
    };
}

#endif
