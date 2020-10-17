#ifndef GAME_SCRIPT_COMPILERCONTEXT_H
#define GAME_SCRIPT_COMPILERCONTEXT_H

#include <components/compiler/context.hpp>

namespace MWScript
{
    class CompilerContext : public Compiler::Context
    {
        public:

            enum Type
            {
                Type_Full, // global, local, targeted
                Type_Dialogue,
                Type_Console
            };

        private:

            Type mType;

        public:

            CompilerContext (Type type);

            /// Is the compiler allowed to declare local variables?
            bool canDeclareLocals() const override;

            /// 'l: long, 's': short, 'f': float, ' ': does not exist.
            char getGlobalType (const std::string& name) const override;

            std::pair<char, bool> getMemberType (const std::string& name,
                const std::string& id) const override;
            ///< Return type of member variable \a name in script \a id or in script of reference of
            /// \a id
            /// \return first: 'l: long, 's': short, 'f': float, ' ': does not exist.
            /// second: true: script of reference

            bool isId (const std::string& name) const override;
            ///< Does \a name match an ID, that can be referenced?

            bool isJournalId (const std::string& name) const override;
            ///< Does \a name match a journal ID?
    };
}

#endif
