#include "world.hpp"

#include <MyGUI_InputManager.h>

#include "../sol.hpp"
#include "../luamanager.hpp"
#include "../luautil.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/windowmanager.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwmechanics/weapontype.hpp"

#include "../../mwworld/class.hpp"
#include "../../mwworld/esmstore.hpp"

namespace mwse
{
    namespace lua
    {
        void bindTES3World()
        {
            auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            state["omw"]["getGlobal"] = [](const char* id) -> sol::optional<float>
            {
                return MWBase::Environment::get().getWorld()->getGlobalFloat(id);
            };

            state["omw"]["setGlobal"] = [](const char* id, double value)
            {
                MWBase::Environment::get().getWorld()->setGlobalFloat (id, value);
            };

             state["omw"]["getWeaponType"] = [](int id) -> sol::object
            {
                ESM::WeaponType *weaponType = const_cast<ESM::WeaponType*>(MWMechanics::getWeaponType(id));

                return makeLuaObject(weaponType);
            };

            state["omw"]["addWeaponType"] = [](sol::table params)
            {
                ESM::WeaponType weaponType;
                weaponType.mId = getOptionalParam<int>(params, "id", -1);
                sol::optional<std::string> shortGroup = params["shortGroup"];
                weaponType.mShortGroup = shortGroup.value();
                sol::optional<std::string> longGroup = params["longGroup"];
                weaponType.mLongGroup = longGroup.value();
                sol::optional<std::string> sound = params["sound"];
                weaponType.mSoundId = sound.value();
                sol::optional<std::string> attachBone = params["attachBone"];
                weaponType.mAttachBone = attachBone.value();
                sol::optional<std::string> sheathBone = params["sheathBone"];
                weaponType.mSheathingBone = sheathBone.value();
                weaponType.mSkill = ESM::Skill::SkillEnum(getOptionalParam<int>(params, "skill", 0));
                weaponType.mWeaponClass = ESM::WeaponType::Class(getOptionalParam<int>(params, "class", 0));
                weaponType.mAmmoType = getOptionalParam<int>(params, "ammoType", -1);
                weaponType.mFlags = 0;
                weaponType.mFlags |= getOptionalParam<bool>(params, "twoHanded", true) ? ESM::WeaponType::TwoHanded : 0;
                weaponType.mFlags |= getOptionalParam<bool>(params, "hasHealth", true) ? ESM::WeaponType::HasHealth : 0;

                MWMechanics::registerWeaponType(weaponType);

                return makeLuaObject(&weaponType);
            };

            state["omw"]["getMagicEffect"] = [](int id) -> sol::object
            {
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                ESM::MagicEffect *effect = const_cast<ESM::MagicEffect*>(store.get<ESM::MagicEffect>().find(id));

                return makeLuaObject(effect);
            };

            state["omw"]["getDaysInMonth"] = [](int month) -> sol::optional<int>
            {
                return MWBase::Environment::get().getWorld()->getDaysPerMonth(month);
            };

            state["omw"]["getSimulationTimestamp"] = []() -> sol::optional<float>
            {
                auto timestamp = MWBase::Environment::get().getWorld()->getTimeStamp();
                float time = timestamp.getDay() * 24 + timestamp.getHour();
                return time;
            };
        }
    }
}
