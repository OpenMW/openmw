#ifndef PATHGRIDCREATOR_HPP
#define PATHGRIDCREATOR_HPP

#include "genericcreator.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class Data;
    class IdCompletionManager;
    class IdTable;
    class UniversalId;
}

namespace CSVWidget
{
    class DropLineEdit;
}

namespace CSVWorld
{
    /// \brief Record creator for pathgrids.
    class PathgridCreator : public GenericCreator
    {
        Q_OBJECT

        CSVWidget::DropLineEdit *mCell;

        private:

            /// \return Cell ID entered by user.
            std::string getId() const override;

            /// \return reference to table containing pathgrids.
            CSMWorld::IdTable& getPathgridsTable() const;

        public:

            PathgridCreator(
                CSMWorld::Data& data,
                QUndoStack& undoStack,
                const CSMWorld::UniversalId& id,
                CSMWorld::IdCompletionManager& completionManager);

            /// \brief Set cell ID input widget to ID of record to be cloned.
            /// \param originId Cell ID to be cloned.
            /// \param type Type of record to be cloned.
            void cloneMode(
                const std::string& originId,
                const CSMWorld::UniversalId::Type type) override;

            /// \return Error description for current user input.
            std::string getErrors() const override;

            /// \brief Set focus to cell ID input widget.
            void focus() override;

            /// \brief Clear cell ID input widget.
            void reset() override;

        private slots:

            /// \brief Check user input for errors.
            void cellChanged();
    };

    /// \brief Creator factory for pathgrid record creator.
    class PathgridCreatorFactory : public CreatorFactoryBase
    {
        public:

            Creator *makeCreator(
                CSMDoc::Document& document,
                const CSMWorld::UniversalId& id) const override;
    };
}

#endif // PATHGRIDCREATOR_HPP
