#include "types.hpp"

#include "../luabindings.hpp"

namespace MWLua
{

    static const MWWorld::Ptr& containerPtr(const Object& o) { return verifyType(ESM::REC_CONT, o.ptr()); }

    void addContainerBindings(sol::table container, const Context& context)
    {
        container["content"] = sol::overload(
            [](const LObject& o) { containerPtr(o); return Inventory<LObject>{o}; },
            [](const GObject& o) { containerPtr(o); return Inventory<GObject>{o}; }
        );
    }

}
