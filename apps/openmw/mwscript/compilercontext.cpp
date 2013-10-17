
#include "compilercontext.hpp"

#include "../mwworld/esmstore.hpp"

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
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        return
            store.get<ESM::Activator>().search (name) ||
            store.get<ESM::Potion>().search (name) ||
            store.get<ESM::Apparatus>().search (name) ||
            store.get<ESM::Armor>().search (name) ||
            store.get<ESM::Book>().search (name) ||
            store.get<ESM::Clothing>().search (name) ||
            store.get<ESM::Container>().search (name) ||
            store.get<ESM::Creature>().search (name) ||
            store.get<ESM::Door>().search (name) ||
            store.get<ESM::Ingredient>().search (name) ||
            store.get<ESM::CreatureLevList>().search (name) ||
            store.get<ESM::ItemLevList>().search (name) ||
            store.get<ESM::Light>().search (name) ||
            store.get<ESM::Lockpick>().search (name) ||
            store.get<ESM::Miscellaneous>().search (name) ||
            store.get<ESM::NPC>().search (name) ||
            store.get<ESM::Probe>().search (name) ||
            store.get<ESM::Repair>().search (name) ||
            store.get<ESM::Static>().search (name) ||
            store.get<ESM::Weapon>().search (name);
    }
}
