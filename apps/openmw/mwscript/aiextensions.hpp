#ifndef GAME_SCRIPT_AIEXTENSIONS_H
#define GAME_SCRIPT_AIEXTENSIONS_H

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    /// \brief AI-related script functionality
    namespace Ai
    {
        void installOpcodes(Interpreter::Interpreter& interpreter);
    }
}

#endif
