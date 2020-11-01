#ifndef CSV_WORLD_INFOCREATOR_H
#define CSV_WORLD_INFOCREATOR_H

#include "genericcreator.hpp"

namespace CSMWorld
{
    class InfoCollection;
    class IdCompletionManager;
}

namespace CSVWidget
{
    class DropLineEdit;
}

namespace CSVWorld
{
    class InfoCreator : public GenericCreator
    {
            Q_OBJECT

            CSVWidget::DropLineEdit *mTopic;

            std::string getId() const override;

            void configureCreateCommand (CSMWorld::CreateCommand& command) const override;

        public:

            InfoCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id, CSMWorld::IdCompletionManager& completionManager);

            void cloneMode (const std::string& originId,
                const CSMWorld::UniversalId::Type type) override;

            void reset() override;

            std::string getErrors() const override;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.
            
            /// Focus main input widget
            void focus() override;
            
        private slots:

            void topicChanged();
    };

    class InfoCreatorFactory : public CreatorFactoryBase
    {
        public:

            Creator *makeCreator (CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
            ///< The ownership of the returned Creator is transferred to the caller.
    };
}

#endif
