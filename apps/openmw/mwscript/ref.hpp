#ifndef GAME_MWSCRIPT_REF_H
#define GAME_MWSCRIPT_REF_H

#include <string>

#include <components/interpreter/runtime.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/ptr.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    struct ExplicitRef
    {
        MWWorld::Ptr operator() (Interpreter::Runtime& runtime, bool required=true) const
        {
            std::string id = runtime.getStringLiteral (runtime[0].mInteger);
            runtime.pop();

            return MWBase::Environment::get().getWorld()->getPtr (id, false);
        }
    };

    struct ImplicitRef
    {
        MWWorld::Ptr operator() (Interpreter::Runtime& runtime, bool required=true) const
        {
            MWScript::InterpreterContext& context
                = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

            return context.getReference(required);
        }
    };
}

#endif
