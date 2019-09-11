#include "skill.hpp"

#include <osg/Vec4f>

#include "../luamanager.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwworld/esmstore.hpp"

namespace MWLua
{
    void bindTES3Skill()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Skill>();

        usertypeDefinition.set("new", sol::no_constructor);

        // Basic property binding.
        usertypeDefinition.set("id", sol::readonly_property(&ESM::Skill::mIndex));
        usertypeDefinition.set("attribute", sol::property(
            [](ESM::Skill& self) { return self.mData.mAttribute; },
            [](ESM::Skill& self, float value) { self.mData.mAttribute = value; }
        ));
        usertypeDefinition.set("specialization", sol::property(
            [](ESM::Skill& self) { return self.mData.mSpecialization; },
            [](ESM::Skill& self, float value) { self.mData.mSpecialization = value; }
        ));

        // Functions as properties.
        usertypeDefinition.set("name", sol::readonly_property(
            [](const ESM::Skill& self)
            {
                auto store = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
                return store.find(ESM::Skill::sSkillNameIds[self.mIndex])->mValue.getString();
            }
        ));
        usertypeDefinition.set("iconPath", sol::readonly_property(
            [](const ESM::Skill& self)
            {
                std::string icon = "icons\\k\\" + ESM::Skill::sIconNames[self.mIndex];
                return icon;
            }
        ));

        // Indirect bindings to unions and arrays.
        usertypeDefinition.set("actions", sol::readonly_property([](ESM::Skill& self) { return osg::Vec4f(self.mData.mUseValue[0], self.mData.mUseValue[1], self.mData.mUseValue[2], self.mData.mUseValue[3]); }));

        // Finish up our usertype.
        state.set_usertype("tes3skill", usertypeDefinition);
    }
}
