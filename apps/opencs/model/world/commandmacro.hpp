#ifndef CSM_WOLRD_COMMANDMACRO_H
#define CSM_WOLRD_COMMANDMACRO_H

class QUndoStack;
class QUndoCommand;

#include <QString>

namespace CSMWorld
{
    class CommandMacro
    {
            QUndoStack& mUndoStack;
            QString mDescription;
            bool mStarted;

            /// not implemented
            CommandMacro (const CommandMacro&);

            /// not implemented
            CommandMacro& operator= (const CommandMacro&);

        public:

            /// If \a description is empty, the description of the first command is used.
            CommandMacro (QUndoStack& undoStack, const QString& description = "");

            ~CommandMacro();

            void push (QUndoCommand *command);
    };
}

#endif
