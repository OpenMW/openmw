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
            virtual std::string getId() const;

        public:

            BodyPartCreator(
                CSMWorld::Data& data,
                QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            /// \return Error description for current user input.
            virtual std::string getErrors() const;

            /// \brief Clear ID and checkbox input widgets.
            virtual void reset();

        private slots:

            void checkboxClicked();
    };
}

#endif // BODYPARTCREATOR_HPP
