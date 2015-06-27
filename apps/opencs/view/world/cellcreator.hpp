#ifndef CSV_WORLD_CELLCREATOR_H
#define CSV_WORLD_CELLCREATOR_H

class QLabel;
class QSpinBox;
class QComboBox;

#include "genericcreator.hpp"

namespace CSVWorld
{
    class CellCreator : public GenericCreator
    {
            Q_OBJECT

            QComboBox *mType;
            QLabel *mXLabel;
            QSpinBox *mX;
            QLabel *mYLabel;
            QSpinBox *mY;

        protected:

            virtual std::string getId() const;

            /// Allow subclasses to add additional data to \a command.
            virtual void configureCreateCommand(CSMWorld::CreateCommand& command) const;

        public:

            CellCreator (CSMWorld::Data& data, QUndoStack& undoStack, const CSMWorld::UniversalId& id);

            virtual void reset();

            virtual void cloneMode(const std::string& originId, 
                                   const CSMWorld::UniversalId::Type type);

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.

        private slots:

            void setType (int index);

            void valueChanged (int index);
    };
}

#endif
