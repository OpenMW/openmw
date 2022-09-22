#ifndef GAME_MWBASE_SCRIPTMANAGER_H
#define GAME_MWBASE_SCRIPTMANAGER_H

#include <string_view>

namespace Interpreter
{
    class Context;
}

namespace Compiler
{
    class Extensions;
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
        ScriptManager(const ScriptManager&);
        ///< not implemented

        ScriptManager& operator=(const ScriptManager&);
        ///< not implemented

    public:
        ScriptManager() {}

        virtual ~ScriptManager() {}

        virtual void clear() = 0;

        virtual bool run(std::string_view name, Interpreter::Context& interpreterContext) = 0;
        ///< Run the script with the given name (compile first, if not compiled yet)

        virtual bool compile(std::string_view name) = 0;
        ///< Compile script with the given namen
        /// \return Success?

        virtual std::pair<int, int> compileAll() = 0;
        ///< Compile all scripts
        /// \return count, success

        virtual const Compiler::Locals& getLocals(std::string_view name) = 0;
        ///< Return locals for script \a name.

        virtual MWScript::GlobalScripts& getGlobalScripts() = 0;

        virtual const Compiler::Extensions& getExtensions() const = 0;
    };
}

#endif
