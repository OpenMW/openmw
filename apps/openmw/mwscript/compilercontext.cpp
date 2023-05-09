#include "compilercontext.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/compiler/locals.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadalch.hpp>
#include <components/esm3/loadappa.hpp>
#include <components/esm3/loadarmo.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclot.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadingr.hpp>
#include <components/esm3/loadlevlist.hpp>
#include <components/esm3/loadligh.hpp>
#include <components/esm3/loadlock.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadprob.hpp>
#include <components/esm3/loadrepa.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/loadweap.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/ptr.hpp"

namespace MWScript
{
    CompilerContext::CompilerContext(Type type)
        : mType(type)
    {
    }

    bool CompilerContext::canDeclareLocals() const
    {
        return mType == Type_Full;
    }

    char CompilerContext::getGlobalType(const std::string& name) const
    {
        return MWBase::Environment::get().getWorld()->getGlobalVariableType(name);
    }

    std::pair<char, bool> CompilerContext::getMemberType(const std::string& name, const ESM::RefId& id) const
    {
        ESM::RefId script;
        bool reference = false;

        if (const ESM::Script* scriptRecord = MWBase::Environment::get().getESMStore()->get<ESM::Script>().search(id))
        {
            script = scriptRecord->mId;
        }
        else
        {
            MWWorld::ManualRef ref(*MWBase::Environment::get().getESMStore(), id);

            script = ref.getPtr().getClass().getScript(ref.getPtr());
            reference = true;
        }

        char type = ' ';

        if (!script.empty())
            type = MWBase::Environment::get().getScriptManager()->getLocals(script).getType(
                Misc::StringUtils::lowerCase(name));

        return std::make_pair(type, reference);
    }

    bool CompilerContext::isId(const ESM::RefId& name) const
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        return store.get<ESM::Activator>().search(name) || store.get<ESM::Potion>().search(name)
            || store.get<ESM::Apparatus>().search(name) || store.get<ESM::Armor>().search(name)
            || store.get<ESM::Book>().search(name) || store.get<ESM::Clothing>().search(name)
            || store.get<ESM::Container>().search(name) || store.get<ESM::Creature>().search(name)
            || store.get<ESM::Door>().search(name) || store.get<ESM::Ingredient>().search(name)
            || store.get<ESM::CreatureLevList>().search(name) || store.get<ESM::ItemLevList>().search(name)
            || store.get<ESM::Light>().search(name) || store.get<ESM::Lockpick>().search(name)
            || store.get<ESM::Miscellaneous>().search(name) || store.get<ESM::NPC>().search(name)
            || store.get<ESM::Probe>().search(name) || store.get<ESM::Repair>().search(name)
            || store.get<ESM::Static>().search(name) || store.get<ESM::Weapon>().search(name)
            || store.get<ESM::Script>().search(name);
    }
}
