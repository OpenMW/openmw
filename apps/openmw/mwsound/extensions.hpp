#ifndef GAME_SOUND_EXTENSIONS_H
#define GAME_SOUND_EXTENSIONS_H

namespace Compiler
{
    class Extensions;
}

namespace Interpreter
{
    class Interpreter;
}

namespace MWSound
{
    // Script-extensions related to sound
    
    void registerExtensions (Compiler::Extensions& extensions);
    
    void installOpcodes (Interpreter::Interpreter& interpreter);
}

#endif

