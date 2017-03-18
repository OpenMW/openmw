#ifndef PATHGRIDCREATOR_HPP
#define PATHGRIDCREATOR_HPP

class QComboBox;

#include "genericcreator.hpp"

namespace CSVWorld
{
    /// \brief Record creator for pathgrids.
    class PathgridCreator : public GenericCreator
    {
        Q_OBJECT

        QComboBox *mCell;

        private:

            /// \return Cell ID selected by user.
            virtual std::string getId() const;

        public:

            PathgridCreator(
                CSMWorld::Data& data,
                QUndoStack& undoStack,
                const CSMWorld::UniversalId& id,
                bool relaxedIdRules = false);

            /// \return Error description for current user input.
            virtual std::string getErrors() const;

            /// \brief Set focus to cell ID input widget.
            virtual void focus();

            /// \brief Reset selected cell ID.
            virtual void reset();

        private slots:

            /// \brief Check user input for errors.
            void cellChanged();

            /// \brief Setup cells in combo box.
            void setupCellsInput();
    };
}

#endif // PATHGRIDCREATOR_HPP
