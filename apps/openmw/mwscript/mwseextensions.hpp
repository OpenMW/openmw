#ifndef GAME_SCRIPT_MWSEEXTENSIONS_H
#define GAME_SCRIPT_MWSEEXTENSIONS_H

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
    /// \brief GUI-related script functionality
    namespace ScriptExtender
    {
        namespace References {

        }
        namespace Math {
            void installOpcodes (Interpreter::Interpreter& interpreter);
        }
    }
}

#endif
