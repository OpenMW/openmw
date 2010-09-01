#ifndef GAME_SCRIPT_CONTROLEXTENSIONS_H
#define GAME_SCRIPT_CONTROLEXTENSIONS_H

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
    /// \brief player controls-related script functionality
    namespace Control
    {
        void registerExtensions (Compiler::Extensions& extensions);

        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif
