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
        controls["movement"] = &ActorControls::movement;
        controls["sideMovement"] = &ActorControls::sideMovement;
        controls["turn"] = &ActorControls::turn;
        controls["run"] = &ActorControls::run;
        controls["jump"] = &ActorControls::jump;

        sol::usertype<SelfObject> selfAPI =
            context.mLua->sol().new_usertype<SelfObject>("SelfObject", sol::base_classes, sol::bases<LObject>());
        selfAPI[sol::meta_function::to_string] = [](SelfObject& self) { return "openmw.self[" + self.toString() + "]"; };
        selfAPI["object"] = sol::readonly_property([](SelfObject& self) -> LObject { return LObject(self); });
        selfAPI["controls"] = sol::readonly_property([](SelfObject& self) { return &self.mControls; });
        selfAPI["isActive"] = [](SelfObject& self) { return &self.mIsActive; };
        selfAPI["setDirectControl"] = [](SelfObject& self, bool v) { self.mControls.controlledFromLua = v; };
        selfAPI["enableAI"] = [](SelfObject& self, bool v) { self.mControls.disableAI = !v; };
        selfAPI["setEquipment"] = [manager=context.mLuaManager](const SelfObject& obj, sol::table equipment)
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
            manager->addAction(std::make_unique<SetEquipmentAction>(obj.id(), std::move(eqp)));
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

    std::unique_ptr<LocalScripts> LocalScripts::create(LuaUtil::LuaState* lua, const LObject& obj)
    {
        return std::unique_ptr<LocalScripts>(new LocalScripts(lua, obj));
    }

    LocalScripts::LocalScripts(LuaUtil::LuaState* lua, const LObject& obj)
        : LuaUtil::ScriptsContainer(lua, "L" + idToString(obj.id())), mData(obj)
    {
        mData.mControls.controlledFromLua = false;
        mData.mControls.disableAI = false;
        this->addPackage("openmw.self", sol::make_object(lua->sol(), &mData));
        registerEngineHandlers({&mOnActiveHandlers, &mOnInactiveHandlers});
    }

    void LocalScripts::becomeActive()
    {
        mData.mIsActive = true;
        callEngineHandlers(mOnActiveHandlers);
    }
    void LocalScripts::becomeInactive()
    {
        mData.mIsActive = false;
        callEngineHandlers(mOnInactiveHandlers);
    }

}
