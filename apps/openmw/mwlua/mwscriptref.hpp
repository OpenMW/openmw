#ifndef MWSCRIPTREF_H_
#define MWSCRIPTREF_H_

#include <sol/sol.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwscript/globalscripts.hpp"
#include "../mwworld/localscripts.hpp"

#include "object.hpp"

namespace MWLua
{
    struct MWScriptRef
    {
        ESM::RefId mId;
        sol::optional<GObject> mObj;

        MWScript::Locals& getLocals()
        {
            if (mObj)
                return mObj->ptr().getRefData().getLocals();
            else
                return MWBase::Environment::get().getScriptManager()->getGlobalScripts().getLocals(mId);
        }

        bool isRunning() const
        {
            if (mObj.has_value()) // local script
            {
                MWWorld::LocalScripts& localScripts = MWBase::Environment::get().getWorld()->getLocalScripts();
                return localScripts.isRunning(mId, mObj->ptr());
            }

            return MWBase::Environment::get().getScriptManager()->getGlobalScripts().isRunning(mId);
        }
    };
    struct MWScriptVariables
    {
        MWScriptRef mRef;
    };
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::MWScriptRef> : std::false_type
    {
    };
    template <>
    struct is_automagical<MWLua::MWScriptVariables> : std::false_type
    {
    };
    template <>
    struct is_automagical<ESM::Global> : std::false_type
    {
    };
}

#endif // MWSCRIPTREF_H_
