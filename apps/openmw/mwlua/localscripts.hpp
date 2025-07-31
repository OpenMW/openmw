#ifndef MWLUA_LOCALSCRIPTS_H
#define MWLUA_LOCALSCRIPTS_H

#include <memory>
#include <set>
#include <string>
#include <utility>

#include <components/lua/luastate.hpp>
#include <components/lua/scriptscontainer.hpp>

#include "../mwbase/luamanager.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "object.hpp"

namespace MWLua
{
    struct Context;

    struct SelfObject : public LObject
    {
        class CachedStat
        {
        public:
            using Index = std::variant<int, ESM::RefId, std::monostate>;
            using Setter = void (*)(const Index&, std::string_view, const MWWorld::Ptr&, const sol::object&);

            CachedStat(Setter setter, Index index, std::string_view prop)
                : mSetter(setter)
                , mIndex(std::move(index))
                , mProp(std::move(prop))
            {
            }

            void operator()(const MWWorld::Ptr& ptr, const sol::object& object) const
            {
                mSetter(mIndex, mProp, ptr, object);
            }

            bool operator<(const CachedStat& other) const
            {
                return std::tie(mSetter, mIndex, mProp) < std::tie(other.mSetter, other.mIndex, other.mProp);
            }

        private:
            Setter mSetter; // Function that updates a stat's property
            Index mIndex; // Optional index to disambiguate the stat
            std::string_view mProp; // Name of the stat's property
        };

        SelfObject(const LObject& obj)
            : LObject(obj)
            , mIsActive(false)
        {
        }
        MWBase::LuaManager::ActorControls mControls;
        std::map<CachedStat, sol::main_object> mStatsCache;
        bool mIsActive;
    };

    class LocalScripts : public LuaUtil::ScriptsContainer
    {
    public:
        static void initializeSelfPackage(const Context&);
        LocalScripts(LuaUtil::LuaState* lua, const LObject& obj, LuaUtil::ScriptTracker* tracker = nullptr);

        MWBase::LuaManager::ActorControls* getActorControls() { return &mData.mControls; }
        const MWWorld::Ptr& getPtrOrEmpty() const { return mData.ptrOrEmpty(); }

        void setActive(bool active, bool callHandlers = true);
        bool isActive() const override { return mData.mIsActive; }
        void onConsume(const LObject& consumable) { callEngineHandlers(mOnConsumeHandlers, consumable); }
        void onActivated(const LObject& actor) { callEngineHandlers(mOnActivatedHandlers, actor); }
        void onTeleported() { callEngineHandlers(mOnTeleportedHandlers); }
        void onAnimationTextKey(std::string_view groupname, std::string_view key)
        {
            callEngineHandlers(mOnAnimationTextKeyHandlers, groupname, key);
        }
        void onPlayAnimation(std::string_view groupname, const sol::table& options)
        {
            callEngineHandlers(mOnPlayAnimationHandlers, groupname, options);
        }
        void onSkillUse(std::string_view skillId, int useType, float scale)
        {
            callEngineHandlers(mOnSkillUse, skillId, useType, scale);
        }
        void onSkillLevelUp(std::string_view skillId, std::string_view source)
        {
            callEngineHandlers(mOnSkillLevelUp, skillId, source);
        }

        void applyStatsCache();

        // Calls a lua interface on the player's scripts. This call is only meant for use in updating UI elements.
        template <typename T, typename... Args>
        static std::optional<T> callPlayerInterface(
            std::string_view interfaceName, std::string_view identifier, const Args&... args)
        {
            auto player = MWMechanics::getPlayer();
            auto scripts = player.getRefData().getLuaScripts();
            if (scripts)
                return scripts->callInterface<T>(interfaceName, identifier, args...);

            return std::nullopt;
        }

    protected:
        SelfObject mData;

    private:
        EngineHandlerList mOnActiveHandlers{ "onActive" };
        EngineHandlerList mOnInactiveHandlers{ "onInactive" };
        EngineHandlerList mOnConsumeHandlers{ "onConsume" };
        EngineHandlerList mOnActivatedHandlers{ "onActivated" };
        EngineHandlerList mOnTeleportedHandlers{ "onTeleported" };
        EngineHandlerList mOnAnimationTextKeyHandlers{ "_onAnimationTextKey" };
        EngineHandlerList mOnPlayAnimationHandlers{ "_onPlayAnimation" };
        EngineHandlerList mOnSkillUse{ "_onSkillUse" };
        EngineHandlerList mOnSkillLevelUp{ "_onSkillLevelUp" };
    };

}

#endif // MWLUA_LOCALSCRIPTS_H
