#ifndef CSV_WORLD_INFOCREATOR_H
#define CSV_WORLD_INFOCREATOR_H

#include "genericcreator.hpp"

#include <string>

#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/world/creator.hpp>

class QUndoStack;

namespace CSMWorld
{
    class IdCompletionManager;
    class CreateCommand;
    class Data;
}

namespace CSVWidget
{
    class DropLineEdit;
}

namespace CSMDoc
{
    class Document;
}

namespace CSVWorld
{
    class InfoCreator : public GenericCreator
    {
        Q_OBJECT

        CSVWidget::DropLineEdit* mTopic;

        std::string getId() const override;

        void configureCreateCommand(CSMWorld::CreateCommand& command) const override;

    public:
        explicit InfoCreator(CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id,
            CSMWorld::IdCompletionManager& completionManager);

        void cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type) override;

        void reset() override;

        void setText(const std::string& text);

        std::string getErrors() const override;
        ///< Return formatted error descriptions for the current state of the creator. if an empty
        /// string is returned, there is no error.

        /// Focus main input widget
        void focus() override;

    public slots:

        void callReturnPressed();

    private slots:

        void topicChanged();
    };

    class InfoCreatorFactory : public CreatorFactoryBase
    {
    public:
        Creator* makeCreator(CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
        ///< The ownership of the returned Creator is transferred to the caller.
    };
}

#endif
