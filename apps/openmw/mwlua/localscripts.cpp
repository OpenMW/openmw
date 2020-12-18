#include "localscripts.hpp"

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
        selfAPI["setDirectControl"] = [](SelfObject& self, bool v) { self.mControls.controlledFromLua = v; };
    }

    std::unique_ptr<LocalScripts> LocalScripts::create(LuaUtil::LuaState* lua, const LObject& obj)
    {
        return std::unique_ptr<LocalScripts>(new LocalScripts(lua, obj));
    }

    LocalScripts::LocalScripts(LuaUtil::LuaState* lua, const LObject& obj)
        : LuaUtil::ScriptsContainer(lua, "L" + idToString(obj.id())), mData(obj)
    {
        mData.mControls.controlledFromLua = false;
        this->addPackage("openmw.self", sol::make_object(lua->sol(), &mData));
    }

}
