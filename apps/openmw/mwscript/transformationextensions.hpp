#ifndef GAME_SCRIPT_TRANSFORMATIONEXTENSIONS_H
#define GAME_SCRIPT_TRANSFORMATIONEXTENSIONS_H

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
    namespace Transformation
    {        
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
