#ifndef GAME_SCRIPT_ANIMATIONEXTENSIONS_H
#define GAME_SCRIPT_ANIMATIONEXTENSIONS_H

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
    namespace Animation
    {
        void registerExtensions (Compiler::Extensions& extensions);

        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
