#ifndef GAME_MWSCRIPT_REF_H
#define GAME_MWSCRIPT_REF_H

#include <string>

#include <components/interpreter/runtime.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/world.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    struct ExplicitRef
    {
        MWWorld::Ptr operator() (Interpreter::Runtime& runtime) const
        {
            MWScript::InterpreterContext& context
                = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

            std::string id = runtime.getStringLiteral (runtime[0].mInteger);
            runtime.pop();

            return context.getWorld().getPtr (id, false);
        }
    };

    struct ImplicitRef
    {
        MWWorld::Ptr operator() (Interpreter::Runtime& runtime) const
        {
            MWScript::InterpreterContext& context
                = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

            return context.getReference();
        }
    };
}

#endif
