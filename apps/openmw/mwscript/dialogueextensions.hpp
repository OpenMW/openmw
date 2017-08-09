#ifndef GAME_SCRIPT_DIALOGUEEXTENSIONS_H
#define GAME_SCRIPT_DIALOGUEEXTENSIONS_H

namespace Compiler
{
    class Extensions;
}

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    /// \brief Dialogue/Journal-related script functionality
    namespace Dialogue
    {
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
