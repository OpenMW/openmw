#include "localscripts.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwmechanics/aisequence.hpp"
#include "../mwmechanics/aicombat.hpp"

#include "luamanagerimp.hpp"

namespace sol
{
    template <>
    struct is_automagical<MWBase::LuaManager::ActorControls> : std::false_type {};
    template <>
    struct is_automagical<MWLua::LocalScripts::SelfObject> : std::false_type {};
}

namespace MWLua
{

    void LocalScripts::initializeSelfPackage(const Context& context)
    {
        using ActorControls = MWBase::LuaManager::ActorControls;
        sol::usertype<ActorControls> controls = context.mLua->sol().new_usertype<ActorControls>("ActorControls");

#define CONTROL(TYPE, FIELD) sol::property([](const ActorControls& c) { return c.FIELD; },\
                                           [](ActorControls& c, const TYPE& v) { c.FIELD = v; c.mChanged = true; })
        controls["movement"] = CONTROL(float, mMovement);
        controls["sideMovement"] = CONTROL(float, mSideMovement);
        controls["turn"] = CONTROL(float, mTurn);
        controls["run"] = CONTROL(bool, mRun);
        controls["jump"] = CONTROL(bool, mJump);
#undef CONTROL

        sol::usertype<SelfObject> selfAPI =
            context.mLua->sol().new_usertype<SelfObject>("SelfObject", sol::base_classes, sol::bases<LObject>());
        selfAPI[sol::meta_function::to_string] = [](SelfObject& self) { return "openmw.self[" + self.toString() + "]"; };
        selfAPI["object"] = sol::readonly_property([](SelfObject& self) -> LObject { return LObject(self); });
        selfAPI["controls"] = sol::readonly_property([](SelfObject& self) { return &self.mControls; });
        selfAPI["isActive"] = [](SelfObject& self) { return &self.mIsActive; };
        selfAPI["enableAI"] = [](SelfObject& self, bool v) { self.mControls.mDisableAI = !v; };
        selfAPI["setEquipment"] = [context](const SelfObject& obj, sol::table equipment)
        {
            if (!obj.ptr().getClass().hasInventoryStore(obj.ptr()))
            {
                if (!equipment.empty())
                    throw std::runtime_error(ptrToString(obj.ptr()) + " has no equipment slots");
                return;
            }
            SetEquipmentAction::Equipment eqp;
            for (auto& [key, value] : equipment)
            {
                int slot = key.as<int>();
                if (value.is<LObject>())
                    eqp[slot] = value.as<LObject>().id();
                else
                    eqp[slot] = value.as<std::string>();
            }
            context.mLuaManager->addAction(std::make_unique<SetEquipmentAction>(context.mLua, obj.id(), std::move(eqp)));
        };
        selfAPI["getCombatTarget"] = [worldView=context.mWorldView](SelfObject& self) -> sol::optional<LObject>
        {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            MWWorld::Ptr target;
            if (ai.getCombatTarget(target))
                return LObject(getId(target), worldView->getObjectRegistry());
            else
                return {};
        };
        selfAPI["stopCombat"] = [](SelfObject& self)
        {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            ai.stopCombat();
        };
        selfAPI["startCombat"] = [](SelfObject& self, const LObject& target)
        {
            const MWWorld::Ptr& ptr = self.ptr();
            MWMechanics::AiSequence& ai = ptr.getClass().getCreatureStats(ptr).getAiSequence();
            ai.stack(MWMechanics::AiCombat(target.ptr()), ptr);
        };
    }

    LocalScripts::LocalScripts(LuaUtil::LuaState* lua, const LObject& obj, ESM::LuaScriptCfg::Flags autoStartMode)
        : LuaUtil::ScriptsContainer(lua, "L" + idToString(obj.id()), autoStartMode), mData(obj)
    {
        this->addPackage("openmw.self", sol::make_object(lua->sol(), &mData));
        registerEngineHandlers({&mOnActiveHandlers, &mOnInactiveHandlers, &mOnConsumeHandlers});
    }

    void LocalScripts::receiveEngineEvent(const EngineEvent& event)
    {
        std::visit([this](auto&& arg)
        {
            using EventT = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<EventT, OnActive>)
            {
                mData.mIsActive = true;
                callEngineHandlers(mOnActiveHandlers);
            }
            else if constexpr (std::is_same_v<EventT, OnInactive>)
            {
                mData.mIsActive = false;
                callEngineHandlers(mOnInactiveHandlers);
            }
            else
            {
                static_assert(std::is_same_v<EventT, OnConsume>);
                callEngineHandlers(mOnConsumeHandlers, arg.mRecordId);
            }
        }, event);
    }

}
