
#include "commandmacro.hpp"

#include <QUndoStack>
#include <QUndoCommand>

CSMWorld::CommandMacro::CommandMacro (QUndoStack& undoStack, const QString& description)
: mUndoStack (undoStack), mDescription (description), mStarted (false)
{}

CSMWorld::CommandMacro::~CommandMacro()
{
    if (mStarted)
        mUndoStack.endMacro();
}

void CSMWorld::CommandMacro::push (QUndoCommand *command)
{
    if (!mStarted)
    {
        mUndoStack.beginMacro (mDescription.isEmpty() ? command->text() : mDescription);
        mStarted = true;
    }

    mUndoStack.push (command);
}
