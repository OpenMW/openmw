#ifndef GAME_SCRIPT_MISCEXTENSIONS_H
#define GAME_SCRIPT_MISCEXTENSIONS_H

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    namespace Misc
    {
        void installOpcodes(Interpreter::Interpreter& interpreter);
    }
}

#endif
