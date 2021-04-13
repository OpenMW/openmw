#ifndef MWLUA_LOCALSCRIPTS_H
#define MWLUA_LOCALSCRIPTS_H

#include <memory>
#include <set>
#include <string>

#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "../mwbase/luamanager.hpp"

#include "object.hpp"
#include "luabindings.hpp"

namespace MWLua
{

    class LocalScripts : public LuaUtil::ScriptsContainer
    {
    public:
        static std::unique_ptr<LocalScripts> create(LuaUtil::LuaState* lua, const LObject& obj);
        static void initializeSelfPackage(const Context&);

        const MWBase::LuaManager::ActorControls* getActorControls() const { return &mData.mControls; }

        struct SelfObject : public LObject
        {
            SelfObject(const LObject& obj) : LObject(obj), mIsActive(false) {}
            MWBase::LuaManager::ActorControls mControls;
            bool mIsActive;
        };

        void becomeActive();
        void becomeInactive();
    protected:
        LocalScripts(LuaUtil::LuaState* lua, const LObject& obj);
        SelfObject mData;
    private:
        EngineHandlerList mOnActiveHandlers{"onActive"};
        EngineHandlerList mOnInactiveHandlers{"onInactive"};
    };

}

#endif // MWLUA_LOCALSCRIPTS_H
