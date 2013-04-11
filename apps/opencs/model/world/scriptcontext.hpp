#ifndef CSM_WORLD_SCRIPTCONTEXT_H
#define CSM_WORLD_SCRIPTCONTEXT_H

#include <components/compiler/context.hpp>

namespace CSMWorld
{
    class ScriptContext : public Compiler::Context
    {
        public:

            virtual bool canDeclareLocals() const;
            ///< Is the compiler allowed to declare local variables?

            virtual char getGlobalType (const std::string& name) const;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.

            virtual char getMemberType (const std::string& name, const std::string& id) const;
            ///< 'l: long, 's': short, 'f': float, ' ': does not exist.

            virtual bool isId (const std::string& name) const;
            ///< Does \a name match an ID, that can be referenced?
    };
}

#endif
