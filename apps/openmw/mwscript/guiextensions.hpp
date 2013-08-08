#ifndef GAME_SCRIPT_GUIEXTENSIONS_H
#define GAME_SCRIPT_GUIEXTENSIONS_H

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
    namespace Gui
    {        
        void installOpcodes (Interpreter::Interpreter& interpreter);
    }
}

#endif

