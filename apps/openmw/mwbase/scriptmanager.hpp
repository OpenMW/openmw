#ifndef GAME_MWBASE_SCRIPTMANAGER_H
#define GAME_MWBASE_SCRIPTMANAGER_H

#include <string>

namespace Interpreter
{
    class Context;
}

namespace Compiler
{
    class Locals;
}

namespace MWScript
{
    class GlobalScripts;
}

namespace MWBase
{
    /// \brief Interface for script manager (implemented in MWScript)
    class ScriptManager
    {
            ScriptManager (const ScriptManager&);
            ///< not implemented

            ScriptManager& operator= (const ScriptManager&);
            ///< not implemented

        public:

            ScriptManager() {}

            virtual ~ScriptManager() {}

            virtual void run (const std::string& name, Interpreter::Context& interpreterContext) = 0;
            ///< Run the script with the given name (compile first, if not compiled yet)

            virtual bool compile (const std::string& name) = 0;
            ///< Compile script with the given namen
            /// \return Success?

            virtual std::pair<int, int> compileAll() = 0;
            ///< Compile all scripts
            /// \return count, success

            virtual Compiler::Locals& getLocals (const std::string& name) = 0;
            ///< Return locals for script \a name.

            virtual MWScript::GlobalScripts& getGlobalScripts() = 0;

            virtual int getLocalIndex (const std::string& scriptId, const std::string& variable,
                char type) = 0;
            ///< Return index of the variable of the given name and type in the given script. Will
            /// throw an exception, if there is no such script or variable or the type does not match.

    };
}

#endif
