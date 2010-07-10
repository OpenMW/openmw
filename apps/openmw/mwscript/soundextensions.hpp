#ifndef GAME_SCRIPT_SOUNDEXTENSIONS_H
#define GAME_SCRIPT_SOUNDEXTENSIONS_H

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
    namespace Sound
    {
        // Script-extensions related to sound
        
        void registerExtensions (Compiler::Extensions& extensions);
        
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif

