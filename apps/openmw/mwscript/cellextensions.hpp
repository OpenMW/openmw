#ifndef GAME_SCRIPT_CELLEXTENSIONS_H
#define GAME_SCRIPT_CELLEXTENSIONS_H

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
    /// \brief cell-related script functionality
    namespace Cell
    {        
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif


