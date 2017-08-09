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
    /// \brief Container-related script functionality (chests, NPCs, creatures)
    namespace Container
    {
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
