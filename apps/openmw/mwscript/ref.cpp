#include "ref.hpp"

#include <components/interpreter/runtime.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "interpretercontext.hpp"

MWWorld::Ptr MWScript::ExplicitRef::operator() (Interpreter::Runtime& runtime, bool required,
    bool activeOnly) const
{
    std::string id = runtime.getStringLiteral(runtime[0].mInteger);
    runtime.pop();

    if (required)
        return MWBase::Environment::get().getWorld()->getPtr(id, activeOnly);
    else
        return MWBase::Environment::get().getWorld()->searchPtr(id, activeOnly);
}

MWWorld::Ptr MWScript::ImplicitRef::operator() (Interpreter::Runtime& runtime, bool required,
    bool activeOnly) const
{
    MWScript::InterpreterContext& context
    = static_cast<MWScript::InterpreterContext&> (runtime.getContext());

    return context.getReference(required);
}
