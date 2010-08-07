#ifndef GAME_SCRIPT_CONTAINEREXTENSIONS_H
#define GAME_SCRIPT_CONTAINEREXTENSIONS_H

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
    /// \brief stats-related script functionality (creatures and NPCs)
    namespace Container
    {
        void registerExtensions (Compiler::Extensions& extensions);

        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
