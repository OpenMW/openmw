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
    void installOpcodes (Interpreter::Interpreter& interpreter, bool consoleOnly = false);
    ///< \param consoleOnly include console only opcodes
}

#endif
