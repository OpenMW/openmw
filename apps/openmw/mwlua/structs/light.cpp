#include "light.hpp"

#include "../luamanager.hpp"

#include <components/esm/loadligh.hpp>

#include <components/sceneutil/util.hpp>

namespace MWLua
{
    void bindTES3Light()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Start our usertype. We must finish this with state.set_usertype.
        auto usertypeDefinition = state.create_simple_usertype<ESM::Light>();

        usertypeDefinition.set("new", sol::constructors<ESM::Light()>());

        // Basic property binding.
        usertypeDefinition.set("id", &ESM::Light::mId);
        usertypeDefinition.set("icon", sol::property(
            [](ESM::Light& self) { return self.mIcon; },
            [](ESM::Light& self, const char* value) { if (strlen(value) < 32) self.mIcon = value; }
        ));
        usertypeDefinition.set("mesh", &ESM::Light::mModel);
        usertypeDefinition.set("script", &ESM::Light::mScript);
        usertypeDefinition.set("sound", &ESM::Light::mSound);
        usertypeDefinition.set("name", &ESM::Light::mName);

        // Allow object to be converted to strings using their object ID.
        usertypeDefinition.set(sol::meta_function::to_string, &ESM::Light::mId);

        usertypeDefinition.set("flags", sol::property(
            [](ESM::Light& self) { return self.mData.mFlags; },
            [](ESM::Light& self, int value) { self.mData.mFlags = value; }
        ));
        usertypeDefinition.set("value", sol::property(
            [](ESM::Light& self) { return self.mData.mValue; },
            [](ESM::Light& self, int value) { self.mData.mValue = value; }
        ));
        usertypeDefinition.set("weight", sol::property(
            [](ESM::Light& self) { return self.mData.mWeight; },
            [](ESM::Light& self, double value) { self.mData.mWeight = value; }
        ));
        usertypeDefinition.set("time", sol::property(
            [](ESM::Light& self) { return self.mData.mTime; },
            [](ESM::Light& self, int value) { self.mData.mTime = value; }
        ));
        usertypeDefinition.set("radius", sol::property(
            [](ESM::Light& self) { return self.mData.mRadius; },
            [](ESM::Light& self, int value) { self.mData.mRadius = value; }
        ));
        usertypeDefinition.set("color", sol::property(
            [](ESM::Light& self) { return SceneUtil::colourFromRGBA(self.mData.mColor) * 255.f; },
            [](ESM::Light& self, osg::Vec4f& value)
            {
                osg::Vec4f normalized = value / 255.f;
                self.mData.mColor = SceneUtil::colourToRGBA(normalized);
            }
        ));

        // User-friendly access to flags
        usertypeDefinition.set("isDynamic", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::Dynamic) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::Dynamic : self.mData.mFlags &= ~ESM::Light::Dynamic; }
        ));
        usertypeDefinition.set("canCarry", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::Carry) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::Carry : self.mData.mFlags &= ~ESM::Light::Carry; }
        ));
        usertypeDefinition.set("isNegative", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::Negative) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::Negative : self.mData.mFlags &= ~ESM::Light::Negative; }
        ));
        usertypeDefinition.set("flickers", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::Flicker) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::Flicker : self.mData.mFlags &= ~ESM::Light::Flicker; }
        ));
        usertypeDefinition.set("isFire", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::Fire) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::Fire : self.mData.mFlags &= ~ESM::Light::Fire; }
        ));
        usertypeDefinition.set("isOffByDefault", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::OffDefault) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::OffDefault : self.mData.mFlags &= ~ESM::Light::OffDefault; }
        ));
        usertypeDefinition.set("flickers", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::FlickerSlow) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::FlickerSlow : self.mData.mFlags &= ~ESM::Light::FlickerSlow; }
        ));
        usertypeDefinition.set("pulses", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::Pulse) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::Pulse : self.mData.mFlags &= ~ESM::Light::Pulse; }
        ));
        usertypeDefinition.set("pulsesSlowly", sol::property(
            [](ESM::Light& self) { return (self.mData.mFlags & ESM::Light::PulseSlow) != 0; },
            [](ESM::Light& self, bool set) { set ? self.mData.mFlags |= ESM::Light::PulseSlow : self.mData.mFlags &= ~ESM::Light::PulseSlow; }
        ));

        // Finish up our usertype.
        state.set_usertype("tes3light", usertypeDefinition);
    }
}
