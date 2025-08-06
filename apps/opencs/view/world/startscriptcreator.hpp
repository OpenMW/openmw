#ifndef STARTSCRIPTCREATOR_HPP
#define STARTSCRIPTCREATOR_HPP

#include "genericcreator.hpp"

#include <string>

#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/world/creator.hpp>

class QObject;
class QUndoStack;

namespace CSMWorld
{
    class Data;
    class IdCompletionManager;
    class IdTable;
}

namespace CSMDoc
{
    class Document;
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

        CSVWidget::DropLineEdit* mScript;

    private:
        /// \return script ID entered by user.
        std::string getId() const override;

        /// \return reference to table containing start scripts.
        CSMWorld::IdTable& getStartScriptsTable() const;

    public:
        explicit StartScriptCreator(CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id,
            CSMWorld::IdCompletionManager& completionManager);

        /// \brief Set script ID input widget to ID of record to be cloned.
        /// \param originId Script ID to be cloned.
        /// \param type Type of record to be cloned.
        void cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type) override;

        /// \return Error description for current user input.
        std::string getErrors() const override;

        /// \brief Set focus to script ID input widget.
        void focus() override;

        /// \brief Clear script ID input widget.
        void reset() override;

    private slots:

        /// \brief Check user input for any errors.
        void scriptChanged();
    };

    /// \brief Creator factory for start script record creator.
    class StartScriptCreatorFactory : public CreatorFactoryBase
    {
    public:
        Creator* makeCreator(CSMDoc::Document& document, const CSMWorld::UniversalId& id) const override;
    };
}

#endif // STARTSCRIPTCREATOR_HPP
