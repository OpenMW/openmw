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
class QUndoStack;

namespace CSMWorld
{
    class CreateCommand;
    class Data;
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
            QPushButton *mCancel;
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

            /// \brief Insert given widget before Create and Cancel buttons.
            /// \param widget Widget to add to layout.
            /// \param stretched Whether widget should be streched or not.
            void insertBeforeButtons (QWidget *widget, bool stretched);

            virtual std::string getId() const;

            std::string getClonedId() const;

            virtual std::string getIdValidatorResult() const;

            /// Allow subclasses to add additional data to \a command.
            virtual void configureCreateCommand (CSMWorld::CreateCommand& command) const;

            /// Allow subclasses to wrap the create command together with additional commands
            /// into a macro.
            virtual void pushCommand (std::unique_ptr<CSMWorld::CreateCommand> command,
                const std::string& id);

            CSMWorld::Data& getData() const;

            QUndoStack& getUndoStack();

            const CSMWorld::UniversalId& getCollectionId() const;

            std::string getNamespace() const;

            void setEditorMaxLength(int length);

        private:

            void updateNamespace();

            void addScope (const QString& name, CSMWorld::Scope scope,
                const QString& tooltip);

        public:

            GenericCreator (CSMWorld::Data& data, QUndoStack& undoStack,
                const CSMWorld::UniversalId& id, bool relaxedIdRules = false);

            void setEditLock (bool locked) override;

            void reset() override;

            void toggleWidgets (bool active = true) override;

            void cloneMode(const std::string& originId,
                                   const CSMWorld::UniversalId::Type type) override;

            void touch(const std::vector<CSMWorld::UniversalId>& ids) override;

            virtual std::string getErrors() const;
            ///< Return formatted error descriptions for the current state of the creator. if an empty
            /// string is returned, there is no error.

            void setScope (unsigned int scope) override;

            /// Focus main input widget
            void focus() override;

        private slots:

            void textChanged (const QString& text);

            /// \brief Create record if able to after Return key is pressed on input.
            void inputReturnPressed();

            void create();

            void scopeChanged (int index);

            void dataIdListChanged();
    };
}

#endif
