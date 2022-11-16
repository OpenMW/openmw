#ifndef GAME_SCRIPT_CONSOLEEXTENSIONS_H
#define GAME_SCRIPT_CONSOLEEXTENSIONS_H

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    /// \brief Script functionality limited to the console
    namespace Console
    {
        void installOpcodes(Interpreter::Interpreter& interpreter);
    }
}

#endif
