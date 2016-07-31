#include "compilercontext.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/esm/loaddial.hpp>

#include <components/compiler/locals.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/manualref.hpp"

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

    std::pair<char, bool> CompilerContext::getMemberType (const std::string& name,
        const std::string& id) const
    {
        std::string script;
        bool reference = false;

        if (const ESM::Script *scriptRecord =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Script>().search (id))
        {
            script = scriptRecord->mId;
        }
        else
        {
            MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), id);

            script = ref.getPtr().getClass().getScript (ref.getPtr());
            reference = true;
        }

        char type = ' ';

        if (!script.empty())
            type = MWBase::Environment::get().getScriptManager()->getLocals (script).getType (
                Misc::StringUtils::lowerCase (name));

        return std::make_pair (type, reference);
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
            store.get<ESM::Weapon>().search (name) ||
            store.get<ESM::Script>().search (name);
    }

    bool CompilerContext::isJournalId (const std::string& name) const
    {
        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        const ESM::Dialogue *topic = store.get<ESM::Dialogue>().search (name);

        return topic && topic->mType==ESM::Dialogue::Journal;
    }
}
