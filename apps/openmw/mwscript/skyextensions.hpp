#ifndef GAME_SCRIPT_SKYEXTENSIONS_H
#define GAME_SCRIPT_SKYEXTENSIONS_H

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
    /// \brief sky-related script functionality
    namespace Sky
    {
        void registerExtensions (Compiler::Extensions& extensions);
        
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
