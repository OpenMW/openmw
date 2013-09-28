#ifndef GAME_SCRIPT_USEREXTENSIONS_H
#define GAME_SCRIPT_USEREXTENSIONS_H

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
    /// \brief Temporaty script functionality limited to the console
    namespace User
    {
        void registerExtensions (Compiler::Extensions& extensions);

        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
