#ifndef STARTSCRIPTCREATOR_HPP
#define STARTSCRIPTCREATOR_HPP

#include "genericcreator.hpp"

namespace CSMWorld
{
    class IdCompletionManager;
    class IdTable;
}

namespace CSVWidget
{
    class DropLineEdit;
}

namespace CSVWorld
{
    /// \brief Record creator for start scripts.
    class StartScriptCreator : public GenericCreator
    {
        Q_OBJECT

        CSVWidget::DropLineEdit *mScript;

        private:

            /// \return script ID entered by user.
            virtual std::string getId() const;

            /// \return reference to table containing start scripts.
            CSMWorld::IdTable& getStartScriptsTable() const;

        public:

            StartScriptCreator(
                CSMWorld::Data& data,
                QUndoStack& undoStack,
                const CSMWorld::UniversalId& id,
                CSMWorld::IdCompletionManager& completionManager);

            /// \brief Set script ID input widget to ID of record to be cloned.
            /// \param originId Script ID to be cloned.
            /// \param type Type of record to be cloned.
            virtual void cloneMode(
                const std::string& originId,
                const CSMWorld::UniversalId::Type type);

            /// \return Error description for current user input.
            virtual std::string getErrors() const;

            /// \brief Set focus to script ID input widget.
            virtual void focus();

            /// \brief Clear script ID input widget.
            virtual void reset();

        private slots:

            /// \brief Check user input for any errors.
            void scriptChanged();
     };

     /// \brief Creator factory for start script record creator.
     class StartScriptCreatorFactory : public CreatorFactoryBase
     {
        public:

            virtual Creator *makeCreator(
                CSMDoc::Document& document,
                const CSMWorld::UniversalId& id) const;
     };
}

#endif // STARTSCRIPTCREATOR_HPP
