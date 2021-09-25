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
        static void initializeSelfPackage(const Context&);
        LocalScripts(LuaUtil::LuaState* lua, const LObject& obj, ESM::LuaScriptCfg::Flags autoStartMode);

        MWBase::LuaManager::ActorControls* getActorControls() { return &mData.mControls; }

        struct SelfObject : public LObject
        {
            SelfObject(const LObject& obj) : LObject(obj), mIsActive(false) {}
            MWBase::LuaManager::ActorControls mControls;
            bool mIsActive;
        };

        struct OnActive {};
        struct OnInactive {};
        struct OnConsume
        {
            std::string mRecordId;
        };
        using EngineEvent = std::variant<OnActive, OnInactive, OnConsume>;

        void receiveEngineEvent(const EngineEvent&);

    protected:
        SelfObject mData;

    private:
        EngineHandlerList mOnActiveHandlers{"onActive"};
        EngineHandlerList mOnInactiveHandlers{"onInactive"};
        EngineHandlerList mOnConsumeHandlers{"onConsume"};
    };

}

#endif // MWLUA_LOCALSCRIPTS_H
