#ifndef BODYPARTCREATOR_HPP
#define BODYPARTCREATOR_HPP

class QCheckBox;

#include "genericcreator.hpp"

namespace CSMWorld
{
    class Data;
    class UniversalId;
}

namespace CSVWorld
{
    /// \brief Record creator for body parts.
    class BodyPartCreator : public GenericCreator
    {
        Q_OBJECT

        QCheckBox *mFirstPerson;

        private:

            /// \return ID entered by user.
            std::string getId() const override;

        public:

            BodyPartCreator(
                CSMWorld::Data& data,
                QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            /// \return Error description for current user input.
            std::string getErrors() const override;

            /// \brief Clear ID and checkbox input widgets.
            void reset() override;

        private slots:

            void checkboxClicked();
    };
}

#endif // BODYPARTCREATOR_HPP
