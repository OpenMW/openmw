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
            class CachedStat
            {
            public:
                using Setter = void(*)(int, std::string_view, const MWWorld::Ptr&, const sol::object&);
            private:
                Setter mSetter; // Function that updates a stat's property
                int mIndex; // Optional index to disambiguate the stat
                std::string_view mProp; // Name of the stat's property
            public:
                CachedStat(Setter setter, int index, std::string_view prop) : mSetter(setter), mIndex(index), mProp(std::move(prop)) {}

                void operator()(const MWWorld::Ptr& ptr, const sol::object& object) const
                {
                    mSetter(mIndex, mProp, ptr, object);
                }

                bool operator<(const CachedStat& other) const
                {
                    return std::tie(mSetter, mIndex, mProp) < std::tie(other.mSetter, other.mIndex, other.mProp);
                }
            };

            SelfObject(const LObject& obj) : LObject(obj), mIsActive(false) {}
            MWBase::LuaManager::ActorControls mControls;
            std::map<CachedStat, sol::object> mStatsCache;
            bool mIsActive;
        };

        struct OnActive {};
        struct OnInactive {};
        struct OnActivated
        {
           LObject mActivatingActor;
        };
        struct OnConsume
        {
            std::string mRecordId;
        };
        using EngineEvent = std::variant<OnActive, OnInactive, OnConsume, OnActivated>;

        void receiveEngineEvent(const EngineEvent&);

        void applyStatsCache();
    protected:
        SelfObject mData;

    private:
        EngineHandlerList mOnActiveHandlers{"onActive"};
        EngineHandlerList mOnInactiveHandlers{"onInactive"};
        EngineHandlerList mOnConsumeHandlers{"onConsume"};
        EngineHandlerList mOnActivatedHandlers{"onActivated"};
    };

}

#endif // MWLUA_LOCALSCRIPTS_H
