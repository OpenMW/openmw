#ifndef GAME_SCRIPT_EXTENSIONS_H
#define GAME_SCRIPT_EXTENSIONS_H

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
    void registerExtensions (Compiler::Extensions& extensions);
    
    void installOpcodes (Interpreter::Interpreter& interpreter);
}

#endif
