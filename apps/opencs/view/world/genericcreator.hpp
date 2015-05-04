#ifndef CSV_WORLD_GENERICCREATOR_H
#define CSV_WORLD_GENERICCREATOR_H

#include <memory>

#include "../../model/world/universalid.hpp"

#include "creator.hpp"

class QString;
class QPushButton;
class QLineEdit;
class QHBoxLayout;
class QComboBox;
class QLabel;

namespace CSMWorld
{
    class CreateCommand;
}

namespace CSVWorld
{
    class IdValidator;

    class GenericCreator : public Creator
    {
            Q_OBJECT

            CSMWorld::Data& mData;
            QUndoStack& mUndoStack;
            CSMWorld::UniversalId mListId;
            QPushButton *mCreate;
            QLineEdit *mId;
            std::string mErrors;
            QHBoxLayout *mLayout;
            bool mLocked;
            std::string mClonedId;
            CSMWorld::UniversalId::Type mClonedType;
            unsigned int mScopes;
            QComboBox *mScope;
            QLabel *mScopeLabel;
            IdValidator *mValidator;

        protected:
            bool mCloneMode;

        protected:

            void update();

            virtual void setManualEditing (bool enabled);
            ///< Enable/disable manual ID editing (enabled by default).

            void insertAtBeginning (QWidget *widget, bool stretched);

            void insertBeforeButtons (QWidget *widget, bool stretched);

            virtual std::string getId() const;

            /// Allow subclasses to add additional data to \a command.
            virtual void configureCreateCommand (CSMWorld::CreateCommand& command) const;

            /// Allow subclasses to wrap the create command together with additional commands
            /// into a macro.
            virtual void pushCommand (std::auto_ptr<CSMWorld::CreateCommand> command,
                const std::string& id);

            CSMWorld::Data& getData() const;

            QUndoStack& getUndoStack();

            const CSMWorld::UniversalId& getCollectionId() const;

            std::string getNamespace() const;

        private:

            void updateNamespace();

            void addScope (const QString& name, CSMWorld::Scope scope,
                const QString& tooltip);

        public:

            GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id, bool relaxedIdRules = false);

            virtual void setEditLock (bool locked);

            virtual void reset();

            virtual void toggleWidgets (bool active = true);

            virtual void cloneMode(const std::string& originId,
                                   const CSMWorld::UniversalId::Type type);

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.

            virtual void setScope (unsigned int scope);

            /// Focus main input widget
            virtual void focus();

        private slots:

            void textChanged (const QString& text);

            void create();

            void scopeChanged (int index);
    };
}

#endif
