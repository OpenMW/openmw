#ifndef CSV_WORLD_GENERICCREATOR_H
#define CSV_WORLD_GENERICCREATOR_H

class QPushButton;
class QLineEdit;

#include "creator.hpp"

#include "../../model/world/universalid.hpp"

namespace CSVWorld
{
    class GenericCreator : public Creator
    {
            Q_OBJECT

            CSMWorld::Data& mData;
            QUndoStack& mUndoStack;
            CSMWorld::UniversalId mListId;
            QPushButton *mCreate;
            QLineEdit *mId;
            std::string mErrors;
            bool mLocked;

        private:

            void update();

        public:

            GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id);

            virtual void setEditLock (bool locked);

            virtual void reset();

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.

        private slots:

            void textChanged (const QString& text);

            void create();
    };
}

#endif