
#include "compilercontext.hpp"

#include <components/esm_store/store.hpp>

#include <components/compiler/locals.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"

namespace MWScript
{
    CompilerContext::CompilerContext (Type type)
    : mType (type)
    {}

    bool CompilerContext::canDeclareLocals() const
    {
        return mType==Type_Full;
    }

    char CompilerContext::getGlobalType (const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalVariableType (name);
    }

    char CompilerContext::getMemberType (const std::string& name, const std::string& id) const
    {
        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPtr (id, false);

        std::string script = MWWorld::Class::get (ptr).getScript (ptr);

        if (script.empty())
            return ' ';

        return MWBase::Environment::get().getScriptManager()->getLocals (script).getType (name);
    }

    bool CompilerContext::isId (const std::string& name) const
    {
        return
            MWBase::Environment::get().getWorld()->getStore().activators.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().potions.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().appas.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().armors.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().books.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().clothes.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().containers.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().creatures.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().doors.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().ingreds.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().creatureLists.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().itemLists.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().lights.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().lockpicks.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().miscItems.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().npcs.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().probes.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().repairs.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().statics.search (name) ||
            MWBase::Environment::get().getWorld()->getStore().weapons.search (name);
    }
}
